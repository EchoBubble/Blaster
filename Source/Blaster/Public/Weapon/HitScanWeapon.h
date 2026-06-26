// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BlasterWeapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public ABlasterWeapon
{
	GENERATED_BODY()

public:

	virtual void Fire(const FVector& HitLocation) override;
	
protected:

	void PlayImpactEffect(UWorld* World, const FHitResult& InHit) const;
	void SpawnBeamParticle(UWorld* World, const FTransform& SocketTransform, const FVector& Vector) const;

	/*
	 * 霰弹枪
	 */
	//射线检测以及特效等
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, const FTransform& SocketTransform);

private:

	UPROPERTY(EditAnywhere)
	float Damage = 10.f;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> ImpactParticles;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> BeamParticles;


public:

	FORCEINLINE float GetDamage() const { return Damage; }
	
};
