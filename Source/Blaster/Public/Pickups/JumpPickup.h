// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "JumpPickup.generated.h"

UCLASS()
class BLASTER_API AJumpPickup : public APickup
{
	GENERATED_BODY()

public:
	
	AJumpPickup();

protected:

	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	
private:

	UPROPERTY(EditAnywhere)
	float JumZVelocityBuff = 1500.f;

	UPROPERTY(EditAnywhere)
	float JumpBuffTime = 30.f; 

};
