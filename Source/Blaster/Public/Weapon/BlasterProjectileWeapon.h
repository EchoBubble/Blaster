// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BlasterWeapon.h"
#include "BlasterProjectileWeapon.generated.h"

class AProjectile;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterProjectileWeapon : public ABlasterWeapon
{
	GENERATED_BODY()

public:

	virtual void Fire(const FVector& HitLocation) override;
	
private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;
	
};
