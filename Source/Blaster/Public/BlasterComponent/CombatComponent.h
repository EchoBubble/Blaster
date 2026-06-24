// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Blaster/Public/Weapon/WeaponTypes.h"
#include "BlasterType/CombatState.h"
#include "CombatComponent.generated.h"


class ABlasterWeapon;
class ABlasterCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UCombatComponent();

	friend class ABlasterCharacter;//让玩家类可以访问当前类的私有变量
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void EquipWeapon(ABlasterWeapon* WeaponToEquip);
	void SwapWeapons();
	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReload();
	UFUNCTION(BlueprintCallable)
	void MagazineReload(bool bReload);
	void TryFireAfterReload();

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(Replicated)
	bool bFireButtonPressed;

	UFUNCTION()
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void FireButtonPressed(bool bPressed);

	bool IsReloading() const;

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	void PickupAmmo(EWeaponType::Type WeaponType, int32 AmmoAmount);
protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	int32 AmountToReload();

	/*
	 * 装备武器
	 */
	void PlayEquipSound(ABlasterWeapon* WeaponToEquip);
	void AttachActorToBackupPack(AActor* ActorToAttach);
	void EquipPrimaryWeapon(ABlasterWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(ABlasterWeapon* WeaponToEquip);
private:

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<ABlasterWeapon> EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	TObjectPtr<ABlasterWeapon> SecondaryWeapon;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;
	
	/*
	* 自动开火定时器逻辑
	*/
	
	bool bCanFire = true; // 防止玩家疯狂点击鼠标突破射速限制

	FTimerHandle FireTimerHandle;
	
	void Fire();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize TraceHitTarget);

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticastFire(FVector_NetQuantize TraceHitTarget);

	UFUNCTION(Server, Reliable)
	void SetFireButtonPress(bool FirePressed);
	
	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire() const;
	
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;//当前手里这把武器对应的备用弹药数量,用于显示 HUD 的，目前没有武器类型，以后会做

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType::Type, int32> CarriedAmmoMap;//备用弹药总表,无需同步

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 120;//步枪备用弹药的初始值

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 12;//榴弹枪备用弹药的初始值

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 21;//手枪弹药初始值

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 200;//冲锋枪弹药初始值

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 40;//霰弹弹药初始值

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 24;//狙击枪弹药初始值
	
	void InitializeCarriedAmmo();

	UPROPERTY()
	FVector HitTarget;

	/*
	 *  HUD and crosshairs
	 */
	FHUDPackage HUDPackage;
	
	float CrosshairVelocityFactor = 0.f;//根据速度而扩散的比例
	float CrosshairInAirFactor = 0.f;//空中的扩散比例
	float CrosshairAimFactor = 0.f;//瞄准扩散比例
	float CrosshairFireFactor = 0.f;//开火扩散比例

	/*
	 *  Aiming and FOV
	 */

	float DefaultFOV = 0.f;

	float CurrentFOV = 0.f;

	void InterpFOV(float DeltaTime);

	/*
	 *  CombatState
	 */
	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	TEnumAsByte<ECombatState::Type> CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	/*
	 * Shotgun reload
	 */
	void UpdateAmmoValues();

	void UpdateShotgunAmmoValues();

	bool IsReloadingShotgun() const;

public:
	FORCEINLINE ABlasterWeapon* GetEquippedWeapon() const {return EquippedWeapon;};
	FORCEINLINE float GetCurrentFOV() const {return CurrentFOV;};
	FORCEINLINE float GetDefaultFOV() const {return DefaultFOV;};

	bool ShouldSwapWeapons();
};
