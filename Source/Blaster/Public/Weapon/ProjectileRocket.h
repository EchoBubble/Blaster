// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;
/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	
	AProjectileRocket();
	
protected:

	/*virtual void OnHit(
			UPrimitiveComponent* HitComponent,
			AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit) override;*/

	virtual void BeginPlay() override;

	virtual void HandleDamage(AActor* OtherActor, const FHitResult& Hit) override;
	virtual void FinishHit(const FHitResult& Hit) override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleRocketHitEffects();
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY(EditAnywhere, Category = "Rocket Effects")
	float DestroyDelayAfterHit = 2.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RocketMesh;//本项目没做，因为没资产，懒得找

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailSystemComponent;
};
