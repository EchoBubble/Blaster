// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ALobbyGameMode();
	
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Travel")
	TSoftObjectPtr<UWorld> TargetMap;

	UPROPERTY(EditDefaultsOnly, Category = "Travel")
	int32 RequiredPlayers = 2;
};
