// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/HealthPickup.h"
#include "BlasterComponent/BuffComponent.h"
#include "Character/BlasterCharacter.h"


AHealthPickup::AHealthPickup()
{
	bReplicates = true;
}

void AHealthPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		UBuffComponent* Buff = BlasterCharacter->Buff;
		if (Buff)
		{
			Buff->Heal(HealAmount, HealingTime);//告诉玩家恢复量和恢复时间
		}
	}

	Destroy();
}

