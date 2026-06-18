// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "GameFramework/Actor.h"
#include "BlasterWeapon.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class ACasing;
class UWidgetComponent;
class USphereComponent;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
namespace EWeaponStateNamespace
{
	enum Type
	{
		Initial UMETA(DisplayName = "Initial State"),
		Equipped UMETA(DisplayName = "Equipped"),
		EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
		Dropped UMETA(DisplayName = "Dropped")
	};
}

UCLASS()
class BLASTER_API ABlasterWeapon : public AActor
{
	GENERATED_BODY()

public:

	ABlasterWeapon();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> Magazine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;

	void ShowPickupWidget(bool bShowWidget);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override;

	//播放 Impact 特效
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpactEffects(FVector_NetQuantize ImpactPoint, FVector_NetQuantizeNormal ImpactNormal);

	void Dropped();//丢弃武器

	void AddAmmo(const int32& AmmoToAdd);

	UPROPERTY(EditAnywhere, Category = "Weapon Effects|Impact")
	TObjectPtr<USoundBase> EquipSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Effects|Impact")
	TObjectPtr<USoundBase> DrySound;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Effects")
	TObjectPtr<UAnimMontage> FireAnimation;

	/*
	 * outline
	 */
	void EnableCustomDepth(bool bEnable);

	bool bDestroyWeapon = false;
	
protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);
private:

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere)
	TEnumAsByte<EWeaponStateNamespace::Type> CurrentWeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditAnywhere, Category = "Weapon Effects")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Effects|Impact")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, Category = "Weapon Effects|Impact")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Effects")
	FName MuzzleFlashSocketName = FName("MuzzleFlash");

	UPROPERTY(EditAnywhere, Category = "Weapon Effects|Sound")
	float FireSoundVolumeMin = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Weapon Effects|Sound")
	float FireSoundVolumeMax = 0.65f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACasing> CasingClass;

	/*
	 *  Ammo
	 */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo;// 当前武器里剩多少发子弹

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();
	
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;// 弹匣容量，比如 30 发

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterOwnerCharacter;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterOwnerController;

	UPROPERTY(EditAnywhere)
	TEnumAsByte<EWeaponType::Type> WeaponType;

public:
	void SetWeaponState(const EWeaponStateNamespace::Type State);

	virtual void Fire(const FVector& HitLocation);
	void PlayFireEffects();

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsBottom;

	UPROPERTY(EditAnywhere, Category = "Weapon|Zoom")
	float ZoomedFOV = 45.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Zoom")
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = 0.15f;// 开火间隔（决定射速，越小射速越快）

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true; // 是否为全自动武器

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh;};
	FORCEINLINE float GetZoomedFOV() const {return ZoomedFOV;};
	FORCEINLINE float GetZoomInterpSpeed() const {return ZoomInterpSpeed;};
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }
	FORCEINLINE EWeaponType::Type GetWeaponType() const { return WeaponType; };
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

	bool IsEmpty();
	bool IsFull();

	void UpdateHUDAmmo();

public:
	/*
	 * 仅测试 ,后坐力
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
	float RecoilPitchMin = 0.8f;//每次开枪向上抬多少

	UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
	float RecoilPitchMax = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
	float RecoilYawMin = -0.2f;//每次开枪左右随机偏多少

	UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
	float RecoilYawMax = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
	float RecoilRecoveryRatio = 0.5f;//回退多少比例 0.5 就是回退一半

	UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
	float RecoilRecoverySpeed = 8.f;//回退速度

	UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
	float AimRecoilMultiplier = 0.65f;//瞄准时后坐力倍率

	FORCEINLINE float GetRecoilPitchMin() const { return RecoilPitchMin; }
	FORCEINLINE float GetRecoilPitchMax() const { return RecoilPitchMax; }
	FORCEINLINE float GetRecoilYawMin() const { return RecoilYawMin; }
	FORCEINLINE float GetRecoilYawMax() const { return RecoilYawMax; }
	FORCEINLINE float GetRecoilRecoveryRatio() const { return RecoilRecoveryRatio; }
	FORCEINLINE float GetRecoilRecoverySpeed() const { return RecoilRecoverySpeed; }
	FORCEINLINE float GetAimRecoilMultiplier() const { return AimRecoilMultiplier; }
};



