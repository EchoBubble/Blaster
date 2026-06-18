// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:

	APickupSpawnPoint();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<APickup>> PickupClasses;

	UPROPERTY()
	TObjectPtr<APickup> SpawnedPickup;//保存生成的 Actor

	void SpawnPickup();//生成
	void SpawnPickupTimerFinished();//回调函数
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyActor);

	
public:

	virtual void Tick(float DeltaTime) override;

private:

	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin = 0.f;//最短时间
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax = 0.f;
};
