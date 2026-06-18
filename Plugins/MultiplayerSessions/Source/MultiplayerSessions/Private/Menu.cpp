// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Components/Button.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, const TSoftObjectPtr<UWorld>& InMap)
{
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	LobbyMap = InMap;
	if (!LobbyMap.IsNull())
	{
		const FString LobbyMapPath = LobbyMap.ToSoftObjectPath().GetLongPackageName();
		PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyMapPath);
	}
	
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);//让菜单拿到焦点

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeUIOnly InputModeData;
		InputModeData.SetWidgetToFocus(TakeWidget());//把焦点交给这个菜单
		InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(true);
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

void UMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	Button_JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
}

void UMenu::NativeDestruct()
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.RemoveDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.RemoveAll(this);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.RemoveDynamic(this, &ThisClass::OnStartSession);
	}
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful) 
{
	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 5.f,FColor::Yellow,FString(TEXT("Session created successfully!")));
		
		if (UWorld* World = GetWorld())
		{
			if (PathToLobby.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("PathToLobby is empty"));
				return;
			}
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 5.f,FColor::Red,FString(TEXT("Failed to  created Session!")));

		Button_HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccessful) const
{
	if (MultiplayerSessionsSubsystem == nullptr) return;

	if (!bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Find sessions failed"));
		Button_JoinButton->SetIsEnabled(true);
		return;
	}
	
	for (auto Result : SessionResult)
	{
		FString SettingValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingValue);

		GEngine->AddOnScreenDebugMessage(
			-1, 5.f, FColor::Cyan,
			FString::Printf(TEXT("Found MatchType=%s, Expected=%s"), *SettingValue, *MatchType));
		
		if (SettingValue == MatchType)
		{
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}
	Button_JoinButton->SetIsEnabled(true);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No matching session found"));
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result) const
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Join session failed"));
		Button_JoinButton->SetIsEnabled(true);
		return;
	}
	
	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address;
			if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to resolve connect string"));
				return;
			}
			
			if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
			{
				PlayerController->ClientTravel(Address, TRAVEL_Absolute);
			}
		}
		else
		{
			Button_JoinButton->SetIsEnabled(true);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to resolve connect string"));
		}
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful, const bool bCreateSessionAfterDestroy)
{
	if (bWasSuccessful && bCreateSessionAfterDestroy)
	{
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
		}
	}
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}

void UMenu::HostButtonClicked()
{
	Button_HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	Button_JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString(TEXT("Join Button Clicked!")));
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		const FInputModeGameOnly InputModeData;
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(false);
	}
}
