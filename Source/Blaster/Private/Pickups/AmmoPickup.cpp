// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/AmmoPickup.h"

#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"


AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UCombatComponent* CombatComponent = BlasterCharacter->Combat;
		if (CombatComponent)
		{
			CombatComponent->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}

void AAmmoPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

