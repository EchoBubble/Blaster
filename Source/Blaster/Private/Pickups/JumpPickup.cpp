// Fill out your copyright notice in the Description page of Project Settings.

#include "Pickups/JumpPickup.h"

#include "BlasterComponent/BuffComponent.h"
#include "Character/BlasterCharacter.h"


AJumpPickup::AJumpPickup()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		UBuffComponent* Buff = BlasterCharacter->Buff;
		if (Buff)
		{
			Buff->BuffJump(JumZVelocityBuff, JumpBuffTime);
		}
	}

	Destroy();
}



