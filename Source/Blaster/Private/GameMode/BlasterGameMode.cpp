// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/BlasterGameMode.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "GameState/BlasterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BlasterPlayerController.h"
#include "PlayerState/BlasterPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;//不要一进地图就立刻开始比赛。
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);//进入自定义状态得用这个函数
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = WarmupTime + MatchTime + CooldownTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f)
		{
			RestartGame();//重开游戏，但支队服务器窗口有效，因为它涉及 ServerTravel，正式多人测试最好用打包版本
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter->IsEliminated()) return;
	
	ABlasterPlayerState* AttackPlayerState = Cast<ABlasterPlayerState>(AttackerController->PlayerState);
	ABlasterPlayerState* VictimPlayerState = Cast<ABlasterPlayerState>(VictimController->PlayerState);

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	
	if (AttackPlayerState && AttackPlayerState != VictimPlayerState && BlasterGameState)
	{
		AttackPlayerState->AddToScore(1.0f);
		VictimPlayerState->AddToDefeats(1);
		BlasterGameState->UpdateTopScore(AttackPlayerState);
	}
	
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
	if (VictimController)
	{
		VictimController->UnPossess();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();//Reset() 内部会做一些清理，比如让角色和 Controller 分离，类似准备销毁前的解绑操作。
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this,APlayerStart::StaticClass(), PlayerStarts);
		if (PlayerStarts.Num() > 0)
		{
			int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
			RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterPlayerController* BlasterController = Cast<ABlasterPlayerController>(*It);
		if (BlasterController)
		{
			BlasterController->OnMatchStateSet(MatchState);
		}
	}
}
