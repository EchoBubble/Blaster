// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

ALobbyGameMode::ALobbyGameMode()
{
	bUseSeamlessTravel = true;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumberOfPlayers >= RequiredPlayers)
	{
		if (!TargetMap.IsNull())
		{
			const FString MapPath = TargetMap.ToSoftObjectPath().GetLongPackageName();
			GetWorld()->ServerTravel(MapPath + TEXT("?listen"));
		}
	}
}
