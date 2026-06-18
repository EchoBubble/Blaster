// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class UBoxComponent;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	
protected:

	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	UFUNCTION()//这里必须加 UFUNCTION,不然绑定可能出问题
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit);

	virtual void HandleDamage(AActor* OtherActor, const FHitResult& Hit);
	virtual void HandleImpactEffects(const FHitResult& Hit);
	virtual void FinishHit(const FHitResult& Hit);

	void DisableProjectileAfterHit();
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TracerComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;
	
private:

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(EditAnywhere, Category = "Projectile Effects")
	TObjectPtr<UNiagaraSystem> Tracer;

	bool bHit = false;
};
