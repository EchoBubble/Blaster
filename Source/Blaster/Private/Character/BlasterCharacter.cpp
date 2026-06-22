// Fill out your copyright notice in the Description page of Project Settings.


#include "Blaster/Public/Character/BlasterCharacter.h"

#include "Blaster/Blaster.h"
#include "BlasterComponent/BuffComponent.h"
#include "BlasterComponent/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameMode/BlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Weapon/BlasterWeapon.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(FName("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	bUseControllerRotationYaw = false;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(FName("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetRootComponent());

	Combat = CreateDefaultSubobject<UCombatComponent>(FName("CombatComponent"));
	Combat->SetIsReplicated(true);//武器组件比较特殊，不需要注册，调用这个函数即可同步

	Buff = CreateDefaultSubobject<UBuffComponent>(FName("BuffComponent"));
	Buff->SetIsReplicated(true);
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(FName("DissolveTimelineComponent"));
	
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority() || IsLocallyControlled())
	{
		AimOffset(DeltaTime);
		// 本地玩家 / 服务器自己算出来的值，可以直接给动画用
		//防止 AO_Yaw一会因为旋转和转身interp 导致撕裂，这里等于是计算完后的结果
		AO_YawForAnim = AO_Yaw;
	}
	else
	{
		// 模拟代理不要直接使用网络同步来的 AO_Yaw
		// 先在本地平滑一下，再给 AnimBP 用
		SmoothAOForSimProxy(DeltaTime);
	}
	if (IsLocallyControlled())
	{
		HideCharacterIfCameraClose();
	}

	/*
	 * 仅测试
	 */
	if (IsLocallyControlled() && Combat->EquippedWeapon)
	{
		RecoverRecoil(DeltaTime);
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	if (PlayerController == nullptr) PlayerController = Cast<ABlasterPlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()//无需调用
{
	if (PlayerController == nullptr) PlayerController = Cast<ABlasterPlayerController>(GetController());
	if (PlayerController == nullptr)
	{
		return;
	}
	if (Combat == nullptr)
	{
		return;
	}
	if (!IsValid(Combat->EquippedWeapon))
	{
		return;
	}
	PlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
	PlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	//确保尽在服务器执行，且仅在当前地图有 BlasterGameMode 中才行
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && !bEliminated && DefaultWeaponClass)
	{
		ABlasterWeapon* StartingWeapon = World->SpawnActor<ABlasterWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ABlasterCharacter::TryFireAfterReload()
{
	Combat->TryFireAfterReload();
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateHUDHealth();
	
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}

	//UpdateHUDAmmo();
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)//服务器销毁，EquippedWeapon 是复制变量，客户端收到变化后也会销毁
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                      class AController* InstigatorController, AActor* DamageCauser)
{
	if (bEliminated) return;
	if (Health <= 0.f) return;
	
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	if (Health <= 0.f)
	{
		if (ABlasterGameMode* BlasterGame = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
		{
			if (PlayerController == nullptr) PlayerController = Cast<ABlasterPlayerController>(GetController());
			ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGame->PlayerEliminated(this, PlayerController, BlasterPlayerController);
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (Health < LastHealth)//防止加血的时候也播放受击动画
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	if (!IsWeaponEquipped())
	{
		AO_Yaw = 0.f;
		AO_Pitch = 0.f;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	const FRotator AimRotation = GetBaseAimRotation();
	const FRotator BodyRotation(0.f, GetActorRotation().Yaw, 0.f);

	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(
		AimRotation,
		BodyRotation
	);

	AO_Yaw = FMath::Clamp(DeltaRot.Yaw, -120.f, 120.f);//记录的是旋转偏移值
	AO_Pitch = FMath::Clamp(DeltaRot.Pitch, -80.f, 80.f);
	if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = AO_Yaw;//时刻记录当前的 AO_Yaw 偏移值
	}

	bUseControllerRotationYaw = false;
	TurnInPlace(DeltaTime);
	if (Speed > 0.f || bIsInAir)
	{
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
}

void ABlasterCharacter::SmoothAOForSimProxy(float DeltaTime)
{
	const bool bIsTurning = TurningInPlace != ETurningInPlace::ETIP_NotTurning;

	const float TargetYaw = bIsTurning ? 0.f : AO_Yaw;

	const float InterpSpeed = bIsTurning ? 10.f : 15.f;

	AO_YawForAnim = FMath::FInterpTo(
		AO_YawForAnim,
		TargetYaw,
		DeltaTime,
		InterpSpeed
	);
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw >90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 3.f);//记录的值逐渐归零
		AO_Yaw = InterpAO_Yaw;//设置真实的 AO_Yaw 归零，瞄准偏移也会因此而归零
		FRotator CurrentRotation = GetActorRotation();//当前角色的世界朝向
		FRotator TargetRotation(0.f, GetBaseAimRotation().Yaw, 0.f);

		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime,4.f);//插值获取旋转量

		SetActorRotation(NewRotation);

		if (FMath::Abs(AO_Yaw) < 15.f)//当 Yaw 小于一个值时，判定状态为不旋转
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
	}
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->WeaponMesh)
		{
			Combat->EquippedWeapon->WeaponMesh->SetOwnerNoSee(true);
			Combat->EquippedWeapon->Magazine->SetOwnerNoSee(true);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->WeaponMesh)
		{
			Combat->EquippedWeapon->WeaponMesh->SetOwnerNoSee(false);
			Combat->EquippedWeapon->Magazine->SetOwnerNoSee(false);
		}
	}
}

void ABlasterCharacter::SetOverlappingWeapon(ABlasterWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon) OverlappingWeapon->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(ABlasterWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

bool ABlasterCharacter::IsFiring()
{
	return(Combat && Combat->bFireButtonPressed);
}

ECombatState::Type ABlasterCharacter::GetCombatState()
{
	if (!IsWeaponEquipped() && !Combat) return ECombatState::ECS_MAX;
	
	return Combat->CombatState;
}

ABlasterWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (IsWeaponEquipped() && Combat)
	{
		return (Combat->EquippedWeapon);
	}
	return nullptr;
}

FVector ABlasterCharacter::GetHitTarget()
{
	if (IsWeaponEquipped() && Combat)
	{
		return (Combat->HitTarget);
	}

	return FVector::ZeroVector;
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, AO_Yaw, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, AO_Pitch, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, TurningInPlace, COND_SkipOwner);
	DOREPLIFETIME(ABlasterCharacter, Health)
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)//初始化后一般都存在了，这里只是良好的习惯
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeed);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		SpawnDefaultWeapon();
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat == nullptr) return;
	
	ServerEquipButtonPressed();
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat == nullptr) return;
	
	if (OverlappingWeapon)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
	else if (Combat->ShouldSwapWeapons())//如果两把武器都存在，且不是重叠时按下
	{
		Combat->SwapWeapons();
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)//该变量自带复制
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (FireWeaponMontage && IsWeaponEquipped())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(FireWeaponMontage, 1.0f);
			FName SectionName;
			switch (Combat->EquippedWeapon->GetWeaponType())
			{
			case EWeaponType::EWT_AssaultRifle:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_RocketLauncher:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_SubmachineGun:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_Shotgun:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_SniperRifle:
				SectionName = FName("Rifle");
				break;
			default:
				break;
			}
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	if (Combat->IsReloading())return;
	
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (!AnimInstance->Montage_IsPlaying(HitReactMontage))
		{
			AnimInstance->Montage_Play(HitReactMontage, 1.0f);
			FName SectionName("Front");
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(ReloadMontage, 1.0f);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		default:
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::Elim()
{
	DropOrDestroyWeapons();
	
	MulticastElim();

	GetWorldTimerManager().SetTimer(ElimTimer,this, &ABlasterCharacter::ElimTimerFinished,ElimDelay);
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if(Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
}

void ABlasterCharacter::DropOrDestroyWeapon(ABlasterWeapon* Weapon)
{
	if (Weapon == nullptr) return;
	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if (bEliminated) return;
	bEliminated = true;

	if (PlayerController)
	{
		PlayerController->SetHUDWeaponAmmo(0);
		PlayerController->SetHUDCarriedAmmo(0);
	}
	
	// 停止移动
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}

	// 禁用胶囊体碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 启用 Mesh 布娃娃
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->bBlendPhysics = true;

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		GetMesh()->SetMaterial(1, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.5f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 50.f);
	}
	StartDissolve();

	if (IsLocallyControlled() && Combat && Combat->bAiming &&
			Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	if (ABlasterGameMode* BlasterGame = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		BlasterGame->RequestRespawn(this, PlayerController);
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance && DissolveTimeline != nullptr)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);//播放时，每一帧拿到 Curve 当前值后，调用这个函数
	if (DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);//给 Timeline 添加一条 Float 曲线轨道，并告诉它更新时用哪个委托
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::ApplyRecoil(float PitchKick, float YawKick, float RecoveryRatio, float RecoverySpeed)
{
	if (!IsLocallyControlled() || Controller == nullptr) return;

	const float ClampedRecoveryRatio = FMath::Clamp(RecoveryRatio, 0.f, 1.f);

	// 注意：如果你项目里方向反了，就把 -PitchKick 改成 +PitchKick
	const float PitchInput = -PitchKick;

	AddControllerPitchInput(PitchInput);
	AddControllerYawInput(YawKick);

	// 记录需要恢复的量
	// 例如 PitchKick = 2，RecoveryRatio = 0.5
	// 那就恢复 1 度，剩下 1 度作为累计上抬
	RecoilPitchRecoveryRemaining += -PitchInput * ClampedRecoveryRatio;
	RecoilYawRecoveryRemaining += -YawKick * ClampedRecoveryRatio;

	CurrentRecoilRecoverySpeed = RecoverySpeed;
}

void ABlasterCharacter::RecoverRecoil(float DeltaTime)
{
	if (!IsLocallyControlled() || Controller == nullptr) return;
	if (bDisableGameplay) return;

	const float MaxStep = CurrentRecoilRecoverySpeed * DeltaTime;//算出这一帧最多可以恢复多少角度，例如 10*0.016 = 0.16

	if (!FMath::IsNearlyZero(RecoilPitchRecoveryRemaining))
	{
		const float PitchStep = FMath::Clamp(RecoilPitchRecoveryRemaining,-MaxStep, MaxStep);//正方向和负方向的恢复量

		AddControllerPitchInput(PitchStep);
		RecoilPitchRecoveryRemaining -= PitchStep;//减去已经恢复的量直至归零
	}

	if (!FMath::IsNearlyZero(RecoilYawRecoveryRemaining))
	{
		const float YawStep = FMath::Clamp(RecoilYawRecoveryRemaining,-MaxStep, MaxStep);

		AddControllerYawInput(YawStep);
		RecoilYawRecoveryRemaining -= YawStep;
	}
}
