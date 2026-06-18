// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnRep_Score() override;
	UFUNCTION()//必须加
	virtual void OnRep_Defeats();
	
	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

	FORCEINLINE int32 GetDefeats() const {return Defeats;}

private:

	void UpdateHUDScore();
	void UpdateHUDDefeats();

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats = 0;
};

