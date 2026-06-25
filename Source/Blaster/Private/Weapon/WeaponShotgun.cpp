// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponShotgun.h"

#include "Blaster/Blaster.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


AWeaponShotgun::AWeaponShotgun()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeaponShotgun::Fire(const FVector& HitLocation)
{
	ABlasterWeapon::Fire(HitLocation);//跳过 HitScanWeapon 的逻辑，之际用更上一级的武器基础开火逻辑

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket && InstigatorController)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> HitMap;//玩家数量以及被命中的弹丸数量
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitLocation, FireHit, SocketTransform);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter && HasAuthority())
			{
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;//不是添加玩家，而是给命中次数 + 1
				}
				else
				{
					HitMap.Emplace(BlasterCharacter, 1);//第一次击中时添加这个玩家且命中次数设置为 1
				}
			}
		}
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					GetDamage() * HitPair.Value,
					InstigatorController, this, UDamageType::StaticClass());
			}
		}
	}
}

void AWeaponShotgun::ShoutgunTraceEndWithScatter(const FVector& HitLocation, TArray<FVector>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitLocation - TraceStart).GetSafeNormal();//计算起点到目标的方向
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;//朝向目标并走一个单位
	
	
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);//球体范围内随机偏移
		const FVector EndLoc = SphereCenter + RandVector;//从球心开始后随机偏移的位置
		FVector ToEndLoc = EndLoc - TraceStart;//枪口指向球内随机点 EndLoc 的方向向量(也包含距离，可提部分 safe normal 限制)
		ToEndLoc = TraceStart + ToEndLoc.GetSafeNormal() * TRACE_LENGTH;
		
		HitTargets.Add(ToEndLoc);
	}
}



