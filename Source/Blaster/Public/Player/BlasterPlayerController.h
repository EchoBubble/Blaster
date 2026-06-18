// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterGameMode;
class UCharacterOverlay;
class ABlasterHUD;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABlasterPlayerController();

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void InitHUDScore();
	void SetHUDDefeats(int32 Defeats);
	void InitHUDDefeats();
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);

	virtual float GetServerTime();//同步服务器世界时间
	virtual void ReceivedPlayer() override;//尽早地与服务器时间同步

	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleCooldown();
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDTime();

	void PollInit();//初始化数据

	/*
	 *  同步 客户端和服务器 之间的时间
	 */

	//请求当前服务器时间，当请求发送后传入客户端的时间
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//响应ServerRequestServerTime请求，向客户端报告当前服务器时间。
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClient);

	float ClientServerDelta = 0.f;//客户端和服务器时间的差距

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;//查询频率

	float TimeSyncRunningTime = 0.f;//记录 Delta 值

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Cooldown, float Match, float StartingTime);

private:

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> BlasterContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimingAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;
	
	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);

	void JumpPressed(const FInputActionValue& InputActionValue);
	void JumpReleased(const FInputActionValue& InputActionValue);

	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimingButtonPressed();
	void AimingButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();

	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;

	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;

	float MatchTime = 0.f;//比赛时间
	float WarmupTime = 0.f;//等待开始时间
	float CooldownTime = 0.f;//结算时间
	float LevelStartingTime = 0.f;
	uint32 CountdownInt = 0;//倒计时

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	/*
	 * 缓存 CharacterOverlay 的数据
	 */
	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;
	bool bInitializeCharacterOverlay = false;

	float HUDHealth = 0.f;
	float HUDMaxHealth = 0.f;
	float HUDScore = 0.f;
	int32 HUDDefeats = 0;

	/*
	 *缓存初始武器子弹数量
	 */
	int32 HUDWeaponAmmo = 0;
	int32 HUDCarriedAmmo = 0;

	bool bInitializeWeaponAmmo = false;
	bool bInitializeCarriedAmmo = false;
	
	/*
	 * 
	 */
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;
	
};
