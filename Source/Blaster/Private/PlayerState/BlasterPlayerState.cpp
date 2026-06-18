// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	UpdateHUDScore();
}

void ABlasterPlayerState::OnRep_Defeats()
{
	UpdateHUDDefeats();
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	
	UpdateHUDScore();
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;

	UpdateHUDDefeats();
}

void ABlasterPlayerState::UpdateHUDScore()
{
	ABlasterCharacter* CurrentCharacter = Cast<ABlasterCharacter>(GetPawn());

	if (CurrentCharacter == nullptr) return;

	ABlasterPlayerController* CurrentController = Cast<ABlasterPlayerController>(CurrentCharacter->GetController());

	if (CurrentController)
	{
		CurrentController->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::UpdateHUDDefeats()
{
	ABlasterCharacter* CurrentCharacter = Cast<ABlasterCharacter>(GetPawn());

	if (CurrentCharacter == nullptr) return;

	ABlasterPlayerController* CurrentController = Cast<ABlasterPlayerController>(CurrentCharacter->GetController());

	if (CurrentController)
	{
		CurrentController->SetHUDDefeats(Defeats);
	}
}
