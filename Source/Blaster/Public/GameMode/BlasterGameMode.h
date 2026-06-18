// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown;
}

class ABlasterCharacter;
class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	//记录淘汰玩家、淘汰玩家控制器、攻击者的控制器
	virtual void PlayerEliminated(ABlasterCharacter* ElimmedCharacter,ABlasterPlayerController* VictimController,ABlasterPlayerController* AttackerController);

	virtual void RequestRespawn( ACharacter* ElimmedCharacter,AController* ElimmedController);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;//倒计时

public:
	FORCEINLINE float GetCountdownTime() const {return CountdownTime;}
};
