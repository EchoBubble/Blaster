// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "WeaponShotgun.generated.h"

UCLASS()
class BLASTER_API AWeaponShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	
	AWeaponShotgun();

	virtual void Fire(const FVector& HitLocation) override;
	void ShoutgunTraceEndWithScatter(const FVector& HitLocation, TArray<FVector>& HitTargets);

private:
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfPellets = 10;
};
