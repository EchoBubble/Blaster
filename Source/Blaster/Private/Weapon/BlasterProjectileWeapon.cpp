// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BlasterProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void ABlasterProjectileWeapon::Fire(const FVector& HitLocation)
{
	Super::Fire(HitLocation);

	if (!HasAuthority()) return;
	if (WeaponMesh == nullptr) return;
	if (ProjectileClass == nullptr) return;
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);
		FVector ToTarget = HitLocation - SocketTransform.GetLocation();//从枪口指向目标点的向量
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = GetOwner();
			SpawnParameters.Instigator = InstigatorPawn;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<AProjectile>(
				ProjectileClass,
				SocketTransform.GetLocation(),
				TargetRotation,
				SpawnParameters
				);
		}
	}
}
