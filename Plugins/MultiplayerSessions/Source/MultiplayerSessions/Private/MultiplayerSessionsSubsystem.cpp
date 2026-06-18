// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreatSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	
}

void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
	SessionInterface.Reset();//子系统关闭前进行清理弱引用
	Super::Deinitialize();
}

IOnlineSessionPtr UMultiplayerSessionsSubsystem::GetSessionInterface() const
{
	return SessionInterface.Pin();
}

void UMultiplayerSessionsSubsystem::OnCreatSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (GetSessionInterface())
	{
		GetSessionInterface()->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful); 
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(const bool bWasSuccessful)
{
	if (!GetSessionInterface().IsValid()) return;

	GetSessionInterface()->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

	if (!LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	if (GetSessionInterface())
	{
		GetSessionInterface()->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (GetSessionInterface())
	{
		GetSessionInterface()->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	const bool bShouldRecreate = bCreateSessionAfterDestroy;
	bCreateSessionAfterDestroy = false;

	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful, bShouldRecreate);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (GetSessionInterface())
	{
		GetSessionInterface()->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}

	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::CreateSession(const int32 NumPublicConnections, const FString& MatchType)
{
	if (!GetSessionInterface().IsValid()) return;

	auto ExistingSession = GetSessionInterface()->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		bCreateSessionAfterDestroy = true;
		DestroySession();
		
		return;
	}

	CreateSessionCompleteDelegateHandle = GetSessionInterface()->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->bIsLANMatch = OnlineSubsystem->GetSubsystemName() == "NULL" ? true : false;//true 为局域网，false 为 在线房\平台房
	LastSessionSettings->NumPublicConnections = NumPublicConnections;//公开可加入多少个
	LastSessionSettings->bAllowJoinInProgress = true;//允许他人中途假如
	LastSessionSettings->bAllowJoinViaPresence = true;//允许通过 Persence 方式加入
	LastSessionSettings->bShouldAdvertise = true;//可被别人搜索到
	LastSessionSettings->bUsesPresence = true;//这个 Session 会使用 Presence 相关能力。
	LastSessionSettings->bUseLobbiesIfAvailable = true;//“如果当前平台支持 Lobby，就优先用 Lobby 这套接口来建房。”
	//给这个房间塞一条自定义键值对，参一：键名，自定义名字。参二：值，真正存的东西，例如类型是这个。参三：设置要不要对外公开以及怎么公开
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;//给这次构建打一个“版本标记 / 房间筛选标记”

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	//参数一：创建者的唯一网络 ID；二：内部会话名字；三：配置表。此外第一参数和第三参数要 &，第一个由于变量是*，所以得解引用，第三个因为是智能指针，所以得用*取出来
	const bool bStarted =
		GetSessionInterface()->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings);

	if (!bStarted)
	{
		GetSessionInterface()->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(const int32 MaxSearchResults)
{
	if (!GetSessionInterface().IsValid()) return;

	FindSessionsCompleteDelegateHandle = GetSessionInterface()->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = OnlineSubsystem->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	//参数二，要的是TSharedRef，所以要转化
	if (!GetSessionInterface()->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		GetSessionInterface()->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!GetSessionInterface().IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = GetSessionInterface()->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!GetSessionInterface()->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		GetSessionInterface()->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
	
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	
	DestroySessionCompleteDelegateHandle = GetSessionInterface()->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!GetSessionInterface()->DestroySession(NAME_GameSession))
	{
		GetSessionInterface()->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false, false);
	}
	
}

//没用到，但写的话大概就是这样
void UMultiplayerSessionsSubsystem::StartSession()
{
	if (GetSessionInterface())
	{
		StartSessionCompleteDelegateHandle = GetSessionInterface()->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

		if (!GetSessionInterface()->StartSession(NAME_GameSession))
		{
			GetSessionInterface()->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
			MultiplayerOnStartSessionComplete.Broadcast(false);
		}
	}
}


