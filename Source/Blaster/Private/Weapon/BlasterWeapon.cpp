// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BlasterWeapon.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Weapon/Casing.h"


ABlasterWeapon::ABlasterWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//不管有没有被摄像机看到，都强制让它每帧更新动画姿势和骨骼位置，避免客户端因为远距离导致枪口位置发生变化
	WeaponMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	Magazine = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Magazine"));
	Magazine->SetupAttachment(WeaponMesh, FName("MagazineSocket"));
	Magazine->SetCollisionResponseToAllChannels(ECR_Block);
	Magazine->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Magazine->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	AreaSphere = CreateDefaultSubobject<USphereComponent>(FName("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(FName("Pickup"));
	PickupWidget->SetupAttachment(GetRootComponent());
}

void ABlasterWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void ABlasterWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterWeapon, CurrentWeaponState);
	DOREPLIFETIME(ABlasterWeapon, Ammo);
}

void ABlasterWeapon::MulticastImpactEffects_Implementation(FVector_NetQuantize ImpactPoint,
                                                           FVector_NetQuantizeNormal ImpactNormal)
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			ImpactEffect,
			ImpactPoint,
			ImpactNormal.Rotation(),
			FVector::OneVector
		);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ImpactSound,
			ImpactPoint
		);
	}
}

void ABlasterWeapon::Dropped()
{
	SetWeaponState(EWeaponStateNamespace::Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void ABlasterWeapon::AddAmmo(const int32& AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	UpdateHUDAmmo();
}

void ABlasterWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void ABlasterWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
}

void ABlasterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABlasterWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void ABlasterWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void ABlasterWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	UpdateHUDAmmo();
}

bool ABlasterWeapon::IsEmpty()
{
	if (Ammo <= 0) UGameplayStatics::PlaySoundAtLocation(this, DrySound, GetActorLocation());
	return Ammo <= 0;
}

bool ABlasterWeapon::IsFull()
{
	return Ammo == MagCapacity;
}

void ABlasterWeapon::UpdateHUDAmmo()
{
	if (!IsValid(BlasterOwnerCharacter))
	{
		BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());//这里的 Owner 就是玩家，在装备武器的时候设置了
	}
	// 关键：GetOwner() 可能为空，尤其是死亡 Dropped 后
	if (!IsValid(BlasterOwnerCharacter))
	{
		return;
	}
	if (BlasterOwnerController == nullptr)
	{
		BlasterOwnerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->GetController());
	}
	if (!IsValid(BlasterOwnerController))
	{
		return;
	}
	if (!BlasterOwnerController->IsLocalController())//因为其它端只有自己的控制器，如果不限制，其它端上看到的实例将因为是空指针而崩溃
	{
		return;
	}
	BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
}

void ABlasterWeapon::OnRep_Ammo()
{
	if (BlasterOwnerCharacter == nullptr) BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	
	if (BlasterOwnerCharacter &&
		BlasterOwnerCharacter->Combat && IsFull() &&
		BlasterOwnerCharacter->Combat->GetEquippedWeapon()->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		BlasterOwnerCharacter->Combat->JumpToShotgunEnd();
	}
	
	UpdateHUDAmmo();

	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		OwnerCharacter->TryFireAfterReload();
	}
}

void ABlasterWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		if (BlasterOwnerCharacter == nullptr)
		{
			BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
		}
		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			UpdateHUDAmmo();//初始武器在前面的判断下调用会因为时序问题而无法更新 HUD，所以需要在 OnRep_EquippedWeapon 也进行调用
		}
	}
}

void ABlasterWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void ABlasterWeapon::SetWeaponState(const EWeaponStateNamespace::Type State)
{
	CurrentWeaponState = State;
	OnWeaponStateSet();
}

void ABlasterWeapon::OnWeaponStateSet()
{
	switch (CurrentWeaponState)
	{
	case EWeaponStateNamespace::Equipped:
		OnEquipped();
		break;
	case EWeaponStateNamespace::EquippedSecondary:
		OnEquippedSecondary();
		break;
	case  EWeaponStateNamespace::Dropped:
		OnDropped();
		break;
		
	default:
		break;
	}
}

void ABlasterWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnableCustomDepth(false);
}

void ABlasterWeapon::OnDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}

void ABlasterWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnableCustomDepth(false);
	/*if (WeaponMesh)
	{
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();
	}*/
}

void ABlasterWeapon::Fire(const FVector& HitLocation)
{
	if (WeaponMesh == nullptr) return;

	const FName SocketNameToUse = WeaponMesh->DoesSocketExist(MuzzleFlashSocketName)
		? MuzzleFlashSocketName
		: NAME_None;

	if (MuzzleFlashEffect)
	{
		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlashEffect,
			WeaponMesh,
			SocketNameToUse,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);

		if (NiagaraComponent)
		{
			NiagaraComponent->SetAutoDestroy(true);
		}
	}

	if (FireSound)
	{
		const FVector SoundLocation = WeaponMesh->DoesSocketExist(MuzzleFlashSocketName)
			? WeaponMesh->GetSocketLocation(MuzzleFlashSocketName)
			: GetActorLocation();

		const float RandomVolume = FMath::FRandRange(FireSoundVolumeMin, FireSoundVolumeMax);
		
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSound,
			SoundLocation,
			RandomVolume
		);
	}

	if (CasingClass)
	{
		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			GetWorld()->SpawnActor<ACasing>(
				CasingClass,
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator()
				);
		}
	}

	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	
	if (HasAuthority())
	{
		SpendRound();
	}
}

FVector ABlasterWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();//计算起点到目标的方向
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;//朝向目标并走一个单位
	const FVector RandVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);//球体范围内随机偏移
	const FVector EndLoc = SphereCenter + RandVector;//从球心开始后随机偏移的位置
	const FVector ToEndLoc = EndLoc - TraceStart;//枪口指向球内随机点 EndLoc 的方向向量(也包含距离，可提部分 safe normal 限制)

	/*DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(),
		TraceStart, FVector(TraceStart + ToEndLoc.GetSafeNormal() * TRACE_LENGTH),
		FColor::Cyan, true);*/

	return FVector(TraceStart + ToEndLoc.GetSafeNormal() * TRACE_LENGTH);
}
