// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

class USoundCue;

UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()

public:
	
	ACasing();

protected:
	
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CasingMesh;

	UPROPERTY(EditAnywhere)
	USoundCue* ShellSound;

	bool bHasHit = false;

	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse = 20.f;

	UFUNCTION()//这里必须加 UFUNCTION,不然绑定可能出问题
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit);
};
