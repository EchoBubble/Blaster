// Fill out your copyright notice in the Description page of Project Settings.


#include "Blaster/Public/Player/BlasterPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameMode.h"
#include "GameMode/BlasterGameMode.h"
#include "GameState/BlasterGameState.h"
#include "HUD/Announcement.h"
#include "HUD/CharacterOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/BlasterPlayerState.h"

ABlasterPlayerController::ABlasterPlayerController()
{
	bReplicates = true;	
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(BlasterContext);
	
	SetControlRotation(FRotator(0.f, 0.f, 0.f)); // 永远面向世界 +X
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(BlasterContext, 0);
	}
	
	bShowMouseCursor = false;

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -70.f;
		PlayerCameraManager->ViewPitchMax = 70.f;
	}
	
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();//初始化时间以及创建 announcement 小部件
	
	InitHUDScore();
	InitHUDDefeats();
	/*SetHUDWeaponAmmo(0);
	SetHUDCarriedAmmo(0);*/
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}

	PollInit();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::JumpPressed);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::JumpReleased);
	EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ThisClass::EquipButtonPressed);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::CrouchButtonPressed);
	EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Started, this, &ThisClass::AimingButtonPressed);
	EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Completed, this, &ThisClass::AimingButtonReleased);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
	EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ThisClass::ReloadButtonPressed);
}

void ABlasterPlayerController::Move(const FInputActionValue& InputActionValue)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetDisableGameplay() == true) return;
	}
	
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();//提取移动方向和大小
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}

void ABlasterPlayerController::Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	float FOVScale = 1.f; // 默认缩放倍率为 1（不缩放）

	// 获取当前控制的 Character 和它的 CombatComponent
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->Combat && BlasterCharacter->IsWeaponEquipped() && BlasterCharacter->Combat->bAiming)
	{
		// 计算灵敏度缩放比例：当前 FOV / 默认 FOV
		// 注意：这要求你的 CurrentFOV 和 DefaultFOV 在 CombatComponent.h 中是 public 的，或者有公开的 Getter 方法
		FOVScale = BlasterCharacter->Combat->GetCurrentFOV() / BlasterCharacter->Combat->GetDefaultFOV();
	}

	AddYawInput(LookAxisVector.X * FOVScale);
	AddPitchInput(LookAxisVector.Y * FOVScale);
}

void ABlasterPlayerController::JumpPressed(const FInputActionValue& InputActionValue)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetDisableGameplay() == true) return;
		BlasterCharacter->Jump();
	}
}

void ABlasterPlayerController::JumpReleased(const FInputActionValue& InputActionValue)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetDisableGameplay() == true) return;
		BlasterCharacter->StopJumping();
	}
}

void ABlasterPlayerController::EquipButtonPressed()
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetDisableGameplay() == true) return;
		BlasterCharacter->EquipButtonPressed();
	}
}

void ABlasterPlayerController::CrouchButtonPressed()
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter)
	{
		if (BlasterCharacter->GetDisableGameplay() == true) return;
		BlasterCharacter->CrouchButtonPressed();
	}
}

void ABlasterPlayerController::AimingButtonPressed()
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter == nullptr) return;
	if (BlasterCharacter->GetDisableGameplay() == true) return;
	if (BlasterCharacter->Combat && BlasterCharacter->IsWeaponEquipped())
	{
		BlasterCharacter->Combat->SetAiming(true);
	}
}

void ABlasterPlayerController::AimingButtonReleased()
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter == nullptr) return;
	if (BlasterCharacter->GetDisableGameplay() == true) return;
	if (BlasterCharacter->Combat && BlasterCharacter->IsWeaponEquipped())
	{
		BlasterCharacter->Combat->SetAiming(false);
	}
}

void ABlasterPlayerController::FireButtonPressed()
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter == nullptr) return;
	if (BlasterCharacter->GetDisableGameplay() == true) return;
	if (BlasterCharacter->Combat && BlasterCharacter->IsWeaponEquipped())
	{
		BlasterCharacter->Combat->FireButtonPressed(true);
	}
}

void ABlasterPlayerController::FireButtonReleased()
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter == nullptr) return;
	if (BlasterCharacter->GetDisableGameplay() == true) return;
	if (BlasterCharacter->Combat && BlasterCharacter->IsWeaponEquipped())
	{
		BlasterCharacter->Combat->FireButtonPressed(false);
	}
}

void ABlasterPlayerController::ReloadButtonPressed()
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter == nullptr) return;
	if (BlasterCharacter->GetDisableGameplay() == true) return;
	if (BlasterCharacter->Combat && BlasterCharacter->IsWeaponEquipped())
	{
		BlasterCharacter->Combat->Reload();
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar && BlasterHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::InitHUDScore()
{
	ABlasterPlayerState* BPS = GetPlayerState<ABlasterPlayerState>();
	if (BPS)
	{
		SetHUDScore(BPS->GetScore());
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::InitHUDDefeats()
{
	ABlasterPlayerState* BPS = GetPlayerState<ABlasterPlayerState>();
	if (BPS)
	{
		SetHUDDefeats(BPS->GetDefeats());
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	if (!IsLocalController())
	{
		return;
	}
	
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->AmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	if (!IsLocalController())
	{
		return;
	}
	
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = CarriedAmmo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);//向下取整，计算分钟
		int32 Seconds = CountdownTime - Minutes * 60;//计算秒数
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);//这里的 %02d 意思是：数字不够两位，就在前面补 0
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	bool bHUDValid = BlasterHUD && BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTimer;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);//向下取整，计算分钟
		int32 Seconds = CountdownTime - Minutes * 60;//计算秒数
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);//这里的 %02d 意思是：数字不够两位，就在前面补 0
		BlasterHUD->Announcement->WarmupTimer->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	if (!IsLocalController()) return;

	// 监听服务器主机玩家：每帧从 GameMode 刷新权威时间数据，纯粹是打包版本过早调用缓存，数据还没有，所以这里是兜底
	if (HasAuthority())
	{
		if (BlasterGameMode == nullptr)
		{
			BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		}

		if (BlasterGameMode)
		{
			WarmupTime = BlasterGameMode->WarmupTime;
			MatchTime = BlasterGameMode->MatchTime;
			CooldownTime = BlasterGameMode->CooldownTime;
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
			MatchState = BlasterGameMode->GetMatchState();
		}
	}
	
	float TimeLeft = 0.f;//剩余时间
	
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - (GetServerTime() - LevelStartingTime);
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = (WarmupTime + MatchTime) - (GetServerTime() - LevelStartingTime);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = (WarmupTime + MatchTime + CooldownTime) - (GetServerTime() - LevelStartingTime);
	}
	
	/*if (HasAuthority())//避免监听服务器因为大厅等待等原因计算时间出现偏差
	{
		if (BlasterGameMode == nullptr)
			BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		if (BlasterGameMode)
		{
			TimeLeft = BlasterGameMode->GetCountdownTime() + LevelStartingTime;
		}
	}*/
	
	TimeLeft = FMath::Max(0.f, TimeLeft);
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

//检查 CharacterOverlay 是否有效，有效就把缓存数据设置到 HUD 上
void ABlasterPlayerController::PollInit()
{
	if (!IsLocalController()) return;
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	if (BlasterHUD == nullptr) return;

	if (CharacterOverlay == nullptr)//第一次执行肯定还没有缓存，只有未缓存时才执行
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)//这里才是检查有没有创建这个 widget
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;//缓存这个 widget
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
				
				if (bInitializeWeaponAmmo)
				{
					SetHUDWeaponAmmo(HUDWeaponAmmo);
				}
				if (bInitializeCarriedAmmo)
				{
					SetHUDCarriedAmmo(HUDCarriedAmmo);
				}
			}
		}	
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, CooldownTime, MatchTime, LevelStartingTime);

		/*if (BlasterHUD && MatchState == MatchState::WaitingToStart)//显示热身时间的公示
		{
			BlasterHUD->AddAnnouncement();
		}*/

		OnMatchStateSet(MatchState);
	}
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(const FName StateOfMatch, const float Warmup, const float Cooldown, const float Match, const float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	
	/*if (BlasterHUD && MatchState == MatchState::WaitingToStart)//显示热身时间的公示
	{
		BlasterHUD->AddAnnouncement();
	}*/
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();//当前时间是服务器收到请求那一瞬间的服务器时间
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClient)//一个是客户端发请求时的时间，一个时服务器收到请求时的服务器时间
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;//客户端到服务器再到客户端一共花的时间
	float CurrentServerTime = TimeServerReceivedClient + (0.5f * RoundTripTime);//估算，当时的服务器时间 + 往返时间的一半，估算出当前服务器时间
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();//服务器当前时间 - 客户端当前时间
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

//该函数时机是在 PC 和本地玩家/网络建立好时调用
void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	//if (!HasAuthority()) return;
	MatchState = State;

	if (!IsLocalController()) return;

	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	if (BlasterHUD == nullptr) return;

	if (MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	if (BlasterHUD == nullptr) return;

	if (MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay)
		{
			BlasterHUD->CharacterOverlay->RemoveFromParent();
		}
		if (BlasterHUD->Announcement == nullptr)
		{
			BlasterHUD->AddAnnouncement();
		}
		if (BlasterHUD->Announcement && BlasterHUD->Announcement->Announcement && BlasterHUD->Announcement->InfoText)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			BlasterHUD->Announcement->Announcement->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)//无人得分
				{
					InfoTextString = FString("There is no winner~");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)//只有一个玩家且就是当前玩家
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)//只有一个玩家，但不是当前玩家，显示赢家名字
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)//多玩家并列第一，显示每个赢家的名字
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->Combat)
	{
		BlasterCharacter->bDisableGameplay = true;//此处关闭所有 PC 的输入，例如跳跃、移动、瞄准等
		BlasterCharacter->Combat->FireButtonPressed(false);
	}
}

//该函数时机时 PC 控制某个 Pawn 时启动
void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(
			BlasterCharacter->GetHealth(),
			BlasterCharacter->GetMaxHealth()
		);
	}

	InitHUDScore();
	InitHUDDefeats();
}

void ABlasterPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(P);
	if (BlasterCharacter)
	{
		SetHUDHealth(
			BlasterCharacter->GetHealth(),
			BlasterCharacter->GetMaxHealth()
		);
	}
	InitHUDScore();
	InitHUDDefeats();
}



