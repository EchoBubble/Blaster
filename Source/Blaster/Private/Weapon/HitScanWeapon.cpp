// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "NiagaraFunctionLibrary.h"
#include "Blaster/Blaster.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"

void AHitScanWeapon::Fire(const FVector& HitLocation)
{
	Super::Fire(HitLocation);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket && InstigatorController)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();
		//const FVector End = Start + (HitLocation - Start) * 1.25f;//确保一定击中目标，否则可能只是打在目标表面，检测不到阻挡命中
		
		FHitResult FireHit;
		WeaponTraceHit(Start, HitLocation, FireHit, SocketTransform);
		
		if (FireHit.bBlockingHit)
		{
			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
			{
				if (HasAuthority())//服务器造成伤害
				{
					UGameplayStatics::ApplyDamage(
					BlasterCharacter,Damage, InstigatorController, this, UDamageType::StaticClass());
				}
			}
				
		}
	}
}

void AHitScanWeapon::PlayImpactEffect_Implementation(UWorld* World, const FHitResult& InHit)
{
	if (ImpactParticles)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, ImpactParticles, InHit.ImpactPoint, InHit.ImpactNormal.Rotation(),FVector::OneVector);
	}
}

void AHitScanWeapon::SpawnBeamParticle_Implementation(UWorld* World, const FTransform& SocketTransform, const FVector& Vector)
{
	if (BeamParticles)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(World,BeamParticles, SocketTransform);
		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"), Vector);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, const FTransform& SocketTransform)
{
	if (UWorld* World = GetWorld())
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;//这里的乘法只会改变距离，方向不会更改
		
		World->LineTraceSingleByChannel(OutHit, TraceStart, End, BlasterCollisionChannels::WeaponTrace);

		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
			DrawDebugSphere(GetWorld(), BeamEnd, SphereRadius, 12, FColor::Red, true);
			PlayImpactEffect(World, OutHit);
		}
		SpawnBeamParticle(World, SocketTransform, BeamEnd);
	}
}
