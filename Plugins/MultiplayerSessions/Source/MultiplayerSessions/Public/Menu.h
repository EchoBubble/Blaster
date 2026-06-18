// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UMultiplayerSessionsSubsystem;
class UButton;
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), const TSoftObjectPtr<UWorld>& InMap = nullptr);

protected:
	
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/*
	 * MultiplayerSessionsSubsystem 的委托的回调函数
	 */
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccessful) const;
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result) const;
	
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful, bool bCreateSessionAfterDestroy);
	
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);
private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_HostButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_JoinButton;

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	void MenuTearDown();

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	int32 NumPublicConnections{4};
	FString MatchType{TEXT("FreeForAll")};
	
	UPROPERTY()
	TSoftObjectPtr<UWorld> LobbyMap;

	FString PathToLobby{TEXT("")};
};
