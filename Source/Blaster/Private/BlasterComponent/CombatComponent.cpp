// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/CombatComponent.h"

#include "Blaster/Blaster.h"
#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Weapon/BlasterWeapon.h"


UCombatComponent::UCombatComponent(): bAiming(false), bFireButtonPressed(false), HitTarget()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 300.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->FollowCamera)
		{
			DefaultFOV = Character->FollowCamera->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		SetHUDCrosshairs(DeltaTime);

		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize(0.f, 0.f);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2, ViewportSize.Y / 2);//计算屏幕中心位置
	FVector CrosshairWorldPosition;//屏幕中心转换到世界后的起点位置
	FVector CrosshairWorldDirection;//屏幕中心往世界里看的方向
	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	//将屏幕上的 2D 坐标转换成世界里的 3D 射线
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		PlayerController,
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
		Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Character);
		if (EquippedWeapon)
		{
			QueryParams.AddIgnoredActor(EquippedWeapon);
		}
		
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, BlasterCollisionChannels::WeaponTrace, QueryParams);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
		else
		{
			HitTarget = TraceHitResult.ImpactPoint;
			//DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint, 12.f, 12, FColor::Red);
			if (TraceHitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
			{
				HUDPackage.CrosshairColor = FLinearColor::Red;
			}
			else
			{
				HUDPackage.CrosshairColor = FLinearColor::White;
			}
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr || !Character->IsLocallyControlled()) return;

	ABlasterPlayerController* PlayerController =
		Cast<ABlasterPlayerController>(Character->GetController());

	if (PlayerController == nullptr) return;

	ABlasterHUD* HUD = Cast<ABlasterHUD>(PlayerController->GetHUD());

	if (HUD == nullptr) return;
	
	if (EquippedWeapon)
	{
		HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
		HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
		HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
		HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
		HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
	}
	else
	{
		HUDPackage.CrosshairsCenter = nullptr;
		HUDPackage.CrosshairsLeft = nullptr;
		HUDPackage.CrosshairsRight = nullptr;
		HUDPackage.CrosshairsTop = nullptr;
		HUDPackage.CrosshairsBottom = nullptr;
	}
	//Calculate crosshair spread
	FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
	FVector2D VelocityMultiplierRange(0.f, 1.f);//把速度转换成 0 ~ 1 的扩散系数
	FVector Velocity = Character->GetVelocity();
	Velocity.Z = 0.f;

	// 把角色速度映射成准星扩散值，这个函数才是进行限制的函数，输入范围，输出范围，当前输入值
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange,
		VelocityMultiplierRange,
		Velocity.Size()
	);
	if (Character->GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime,2.25f);
	}
	else
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor,0.f,DeltaTime,30.f);
	}

	if (bAiming)
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.5f, DeltaTime, 30.f);
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}
	if (bFireButtonPressed)
	{
		CrosshairFireFactor = FMath::FInterpTo(CrosshairFireFactor, 0.75f, DeltaTime, 30.f);
	}
	else
	{
		CrosshairFireFactor = FMath::FInterpTo(CrosshairFireFactor, 0.f, DeltaTime, 30.f);
	}
	if (bAiming && Character->GetVelocity().Size() > 1.f)
	{
		HUDPackage.CrosshairSpread = 0.5 + (CrosshairVelocityFactor * 0.5) + CrosshairInAirFactor - CrosshairAimFactor + CrosshairFireFactor;
	}
	else
	{
		HUDPackage.CrosshairSpread = 0.5 + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairFireFactor;
	}
	
	HUD->SetHUDPackage(HUDPackage);
}

void UCombatComponent::SetFireButtonPress_Implementation(bool FirePressed)
{
	bFireButtonPressed = FirePressed;
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	SetFireButtonPress(bPressed);
	if (Character && bPressed)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;

		if (Character)
		{
			// 1. 客户端预测：只要是我自己开的枪，不等服务器，立刻播动画！
			// Listen Server 自己开火不需要预测，因为 ServerFire 会立刻执行
			if (Character->IsLocallyControlled() && !Character->HasAuthority())
			{
				Character->PlayFireMontage(bAiming);
				EquippedWeapon->PlayFireEffects(); // 播放本地的枪口火焰和音效
			}
		}
		// 2. 告诉服务器去干活
		ServerFire(HitTarget);

		if (Character && Character->IsLocallyControlled() && EquippedWeapon)
		{
			const float AimMultiplier = bAiming? EquippedWeapon->GetAimRecoilMultiplier(): 1.f;

			const float PitchKick = FMath::FRandRange(
				EquippedWeapon->GetRecoilPitchMin(),EquippedWeapon->GetRecoilPitchMax()) * AimMultiplier;

			const float YawKick = FMath::FRandRange(
				EquippedWeapon->GetRecoilYawMin(),EquippedWeapon->GetRecoilYawMax()) * AimMultiplier;

			Character->ApplyRecoil(PitchKick,YawKick,EquippedWeapon->GetRecoilRecoveryRatio(),EquippedWeapon->GetRecoilRecoverySpeed()
			);
		}
		
		// 3. 本地开启定时器，控制射速
		StartFireTimer();
	}
	if (EquippedWeapon->GetAmmo() <= 0)
	{
		Reload();
	}
}

void UCombatComponent::ServerFire_Implementation(FVector_NetQuantize TraceHitTarget)
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	const bool bReloadingShotgun = IsReloadingShotgun();
	if (!bReloadingShotgun && CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}
	if (EquippedWeapon->IsEmpty()) return;

	if (bReloadingShotgun)
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}

	Character->PlayFireMontage(bAiming);
	
	// 服务器真正开火：播放服务器这边的表现 + 生成 Projectile
	EquippedWeapon->Fire(TraceHitTarget);
	// 目前没有伤害逻辑，所以服务器唯一的工作就是拿大喇叭广播给其他人
	NetMulticastFire(TraceHitTarget);
}

void UCombatComponent::NetMulticastFire_Implementation(FVector_NetQuantize TraceHitTarget)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	// 服务器已经在 ServerFire_Implementation 里执行过了，避免重复播放/重复逻辑
	if (Character->HasAuthority())
	{
		return;
	}

	// 开火者本地已经预测播放过了，避免播两遍
	if (Character->IsLocallyControlled())
	{
		return;
	}

	//霰弹枪可以直接开火
	const bool bReloadingShotgun = IsReloadingShotgun();

	if (!bReloadingShotgun && CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	if (bReloadingShotgun)
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}

	Character->PlayFireMontage(bAiming);
	EquippedWeapon->PlayFireEffects();
}

void UCombatComponent::StartFireTimer()
{
	if (Character == nullptr || Character->GetWorld() == nullptr) return;

	// 开启定时器，等待 FireDelay 时间后调用 FireTimerFinished
	Character->GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->GetFireDelay()
	);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true; // 定时器结束，解除开火锁定

	// 如果玩家还按着左键，并且武器是全自动的，就继续循环开火
	if (bFireButtonPressed && EquippedWeapon->IsAutomatic())
	{
		if (EquippedWeapon->IsEmpty())
		{
			Reload();
		}
		else
		{
			Fire();
		}
	}
}

bool UCombatComponent::CanFire() const
{
	if (EquippedWeapon == nullptr) return false;

	if (IsReloadingShotgun())
	{
		return !EquippedWeapon->IsEmpty() && bCanFire;
	}

	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;//不为空，或者可以开火的时候才行
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(Character->GetController());
	if (PlayerController)
	{
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	bool bJumpToShotGunEnd = CombatState ==
		ECombatState::ECS_Reloading && EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun && CarriedAmmo == 0;
	if (bJumpToShotGunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	//可简单理解成把一组 key-value 放进 TMap，这里理解成给步枪类型设置初始备用弹药,注意，只有服务器存有数据
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon ==nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	if (Character && Character->FollowCamera)
	{
		Character->FollowCamera->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, bFireButtonPressed, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::PlayEquipSound(ABlasterWeapon* WeaponToEquip)
{
	if (WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipSound, Character->GetActorLocation());
	}
}

void UCombatComponent::AttachActorToBackupPack(AActor* ActorToAttach)
{
	if (ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::EquipWeapon(ABlasterWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else//没有有主武器，副武器可能有可能没有
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::EquipPrimaryWeapon(ABlasterWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
	
	//Character->FollowCamera->SetRelativeRotation(FRotator(0, 10, 0));
	Character->CameraBoom->SocketOffset = FVector(0, 50, 0);
	Character->CameraBoom->TargetArmLength = 130.f;
	
	EquippedWeapon = WeaponToEquip;//从重叠开始，玩家就拥有武器变量，然后这里再赋值
	EquippedWeapon->SetWeaponState(EWeaponStateNamespace::Equipped);
	if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);//网络意义上的 Owner，哪个端调用就是属于哪个端，Owner 是 Actor 自带的赋值变量，客户端也知道；可重构
	EquippedWeapon->UpdateHUDAmmo();

	//更新当前武器类型携带子弹量
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(Character->GetController());
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	if (PlayerController)
	{
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}

	PlayEquipSound(EquippedWeapon);

	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
	
	EquippedWeapon->ShowPickupWidget(false);
}

void UCombatComponent::EquipSecondaryWeapon(ABlasterWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponStateNamespace::Equipped);
	AttachActorToBackupPack(WeaponToEquip);
	PlayEquipSound(SecondaryWeapon);
	SecondaryWeapon->SetOwner(Character);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponStateNamespace::Equipped);
		if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		//Character->FollowCamera->SetRelativeRotation(FRotator(0, 10, 0));
		/*Character->CameraBoom->SocketOffset = FVector(0, 50, 10);
		Character->CameraBoom->TargetArmLength = 130.f;*/
		PlayEquipSound(EquippedWeapon);
		EquippedWeapon->UpdateHUDAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponStateNamespace::Equipped);
		AttachActorToBackupPack(SecondaryWeapon);
		PlayEquipSound(SecondaryWeapon);
	}
}

bool UCombatComponent::IsReloading() const
{
	return CombatState == ECombatState::ECS_Reloading;
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0)
	{
		if (Character && CombatState != ECombatState::ECS_Reloading)
		{
			ServerReload();
		}
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();//计算当前弹夹可装填子弹数量

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];//获取携带弹药数量
		int32 Least = FMath::Min(RoomInMag, AmountCarried);//携带弹药大于可装填时用可装填，如果携带弹药少于可装填空间，用携带弹药
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return int32();
}

void UCombatComponent::UpdateAmmoValues()
{
	int32 ReloadAmount = AmountToReload();//获取可装填的弹药
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;//修改 Map 中携带弹药数量
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];//修改当前武器携带弹药数量
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
	
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(Character->GetController());
	if (PlayerController)
	{
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::FinishReload()
{
	if (!Character) return;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	TryFireAfterReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		/*if (bFireButtonPressed)//客户端不是立刻知道，可能 fire 失败
		{
			Fire();
		}*/
		TryFireAfterReload();
		break;
	default:
		break;
	}
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;//修改 Map 中携带弹药数量
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];//修改当前武器携带弹药数量
	}
	ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(Character->GetController());
	if (PlayerController)
	{
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(1);
	
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)//子弹满了，或者携带弹药为 0，直接跳到 end 触发 finish reload
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

bool UCombatComponent::IsReloadingShotgun() const
{
	return CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun;
}

void UCombatComponent::PickupAmmo(EWeaponType::Type WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);

		if (EquippedWeapon && CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))//防止拾取弹夹和当前武器类型不一致
		{
			CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		}
		if (ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(Character->GetController()))
		{
			PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::TryFireAfterReload()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	if (!Character->IsLocallyControlled()) return;
	if (!bFireButtonPressed) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (EquippedWeapon->IsEmpty()) return;

	if (!EquippedWeapon->IsAutomatic())
	{
		return;
	}
	
	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
	
	Fire();
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;//预测，本地执行，其它端包括服务器不知道，该函数是普通函数，只会在本地端执行
	ServerSetAiming(bIsAiming);//通知服务器权威执行
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
		//这块没必要走 RPC 了，都走本地预测比较合适
		if (bIsAiming)
		{
			Character->CameraBoom->SetRelativeLocation(FVector(0,0,75));
			if (EquippedWeapon->GetWeaponType() != EWeaponType::EWT_SniperRifle)
			{
				Character->FollowCamera->SetRelativeRotation(FRotator(0, -4, 0).Quaternion());
			}
		}
		else
		{
			Character->CameraBoom->SetRelativeLocation(FVector(0,0,65));
			if (EquippedWeapon->GetWeaponType() != EWeaponType::EWT_SniperRifle)
			{
				Character->FollowCamera->SetRelativeRotation(FRotator(0, 0, 0).Quaternion());
			}
		}
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScpeWidget(bIsAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::MagazineReload(bool bReload)
{
	if (bReload)
	{
		if (EquippedWeapon && EquippedWeapon->Magazine)
		{
			EquippedWeapon->Magazine->AttachToComponent
			(Character->GetMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,FName("MagazineSocket"));
		}
	}
	else
	{
		if (EquippedWeapon && EquippedWeapon->Magazine)
		{
			EquippedWeapon->Magazine->AttachToComponent
			(EquippedWeapon->GetWeaponMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,FName("MagazineSocket"));
		}
	}
}