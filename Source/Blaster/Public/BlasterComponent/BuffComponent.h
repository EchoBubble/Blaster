// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


class ABlasterCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	friend class ABlasterCharacter;//让玩家类可以访问当前类的私有变量
	UBuffComponent();
	
	void Heal(float HealAmount, float HealingTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void BuffJump(float BuffJumpVelocity, float BuffTime);
	void SetInitialJumpVelocity(float JumpVelocity);
	
protected:
	virtual void BeginPlay() override;
	void HealRamUp(float DeltaTime);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	/*
	 *  Heal buff
	 */
	bool bHealing = false;
	float HealingRate = 0.f;//每秒回多少血
	float AmountToHeal = 0.f;//剩余还需要恢复多少血

	/*
	 * Speed buff
	 */
	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();
	float InitialBaseSpeed = 0.f;
	float InitialCrouchSpeed = 0.f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/*
	 * Jump buff
	 */
	FTimerHandle JumpBuffTimer;
	void ResetJumpBuff();
	float InitialJumpVelocity = 0.f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpVelocityBuff(float JumpVelocity);
};