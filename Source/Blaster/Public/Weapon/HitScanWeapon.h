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
	UFUNCTION(NetMulticast, Reliable)
	void PlayImpactEffect(UWorld* World, const FHitResult& InHit);

	UFUNCTION(NetMulticast, Reliable)
	void SpawnBeamParticle(UWorld* World, const FTransform& SocketTransform, const FVector& Vector);

	/*
	 * 霰弹枪
	 */
	//返回弹道
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);

	//射线检测以及特效等
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, const FTransform& SocketTransform);

private:

	UPROPERTY(EditAnywhere)
	float Damage = 10.f;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> ImpactParticles;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> BeamParticles;

	/*
	 *  散射射线
	 */

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;//枪口到准星目标方向的延伸，到时候会在这个地方生成一个球体，每次开火会在球里面随机选点作为散射点

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;//球体大小

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

public:

	FORCEINLINE float GetDamage() const { return Damage; }
	
};
