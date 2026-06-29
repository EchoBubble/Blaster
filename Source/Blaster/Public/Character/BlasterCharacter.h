// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/TurningInPlace.h"
#include "Components/TimelineComponent.h"
#include "Interfaces/InteractWithCrosshairInterface.h"
#include "Blaster/Public/BlasterType/CombatState.h"
#include "BlasterCharacter.generated.h"

class UBoxComponent;
class UBuffComponent;
class UTimelineComponent;
class ABlasterPlayerController;
class UCombatComponent;
class ABlasterWeapon;
class UWidgetComponent;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:

	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;

	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION()
	void EquipButtonPressed();

	UFUNCTION()
	void CrouchButtonPressed();

	UFUNCTION()
	void PlayFireMontage(bool bAiming);
	
	void PlayHitReactMontage();

	void PlayReloadMontage();
	
	void Elim();

	void DropOrDestroyWeapon(ABlasterWeapon* Weapon);
	void DropOrDestroyWeapons();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	void TryFireAfterReload();//写在这主要是减少武器的 耦合性

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDAmmo();//无需调用，其实装备武器时就更新了，教程只是多一条路

	void SpawnDefaultWeapon();
	
	/*
	 *  Hit boxes used for server-side rewind
	 */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> head;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> pelvis;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> spine_03;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> spine_05;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> upperarm_l;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> upperarm_r;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> lowerarm_l;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> lowerarm_r;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> hand_l;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> hand_r;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> thigh_l;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> thigh_r;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> calf_l;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> calf_r;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> foot_l;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> foot_r;

	//仅测试
	void ApplyRecoil(float PitchKick, float YawKick, float RecoveryRatio, float RecoverySpeed);

protected:

	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	void AimOffset(float DeltaTime);
	
	UFUNCTION()
	void ReceiveDamage(
		AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<ABlasterWeapon> OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(ABlasterWeapon* LastWeapon);

	// RPC 装备武器
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UPROPERTY(Replicated)
	float AO_Yaw;
	UPROPERTY(Replicated)
	float AO_Pitch;
	UPROPERTY()
	float InterpAO_Yaw = 0.f;
	UPROPERTY()
	float AO_YawForAnim = 0.f;//改成动画蓝图使用这个
	void SmoothAOForSimProxy(float DeltaTime);

	UPROPERTY(Replicated)
	TEnumAsByte<ETurningInPlace::Type> TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/*
	 * Anim montages
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;

	UFUNCTION()
	void HideCharacterIfCameraClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 100.f;//隐藏玩家所需的距离

	/*
	 *  Player Health
	 */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UPROPERTY()
	ABlasterPlayerController* PlayerController;

	/*
	 * Eliminated
	 */
	bool bEliminated = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	
	void ElimTimerFinished();

	/*
	 *  Dissolve effect
	 */

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTimelineComponent> DissolveTimeline;

	FOnTimelineFloat DissolveTrack;//该委托和蓝图 Timeline 的 Update 输出口差不多

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstanceDynamic> DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	/*
	 * Default weapon
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<ABlasterWeapon> DefaultWeaponClass;

public:

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	
	// 武器组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBuffComponent> Buff;
	
	void SetOverlappingWeapon(ABlasterWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	bool IsFiring();

	FORCEINLINE float GetAO_Yaw(){return AO_YawForAnim;}
	FORCEINLINE float GetAO_Pitch(){return AO_Pitch;}
	FORCEINLINE ETurningInPlace::Type GetTurningInPlace() const {return TurningInPlace;}
	FORCEINLINE bool IsEliminated() const {return bEliminated;}
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE void SetHealth(const float Amount) {Health = Amount;}
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	ECombatState::Type GetCombatState();

	UFUNCTION()
	ABlasterWeapon* GetEquippedWeapon();

	FVector GetHitTarget();

private:

	/*
	 * 仅测试
	 */
	void RecoverRecoil(float DeltaTime);

	float RecoilPitchRecoveryRemaining = 0.f;
	float RecoilYawRecoveryRemaining = 0.f;
	float CurrentRecoilRecoverySpeed = 8.f;

	UPROPERTY(EditAnywhere, Category = "Fall Reset")
	float FallResetZ = -5000.f;

	FTransform ServerStartTransform;

	void CheckFallOutOfMap();
	void ResetToStartPosition();

};
