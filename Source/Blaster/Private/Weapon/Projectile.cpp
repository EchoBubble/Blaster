// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Projectile.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/Blaster.h"
#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapon/BlasterWeapon.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetReplicates(true);
	SetReplicateMovement(true);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(FName("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
	
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		TracerComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Tracer,
			CollisionBox,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);

		if (TracerComponent)
		{
			TracerComponent->SetAutoDestroy(true);
		}
		
	}
	if (HasAuthority())
    {
    	CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
    }

	SetLifeSpan(5.f);
}

void AProjectile::Destroyed()
{
	if (TracerComponent)
	{
		TracerComponent->Deactivate();
		TracerComponent->DestroyComponent();
		TracerComponent = nullptr;
	}

	/*if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, ImpactEffect, GetActorLocation(), FRotator::ZeroRotator, FVector::OneVector);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ImpactSound,
			GetActorLocation()
		);
	}*/
	
	Super::Destroyed();
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return;
	if (!HasAuthority()) return;
	if (bHit) return;
	bHit = true;

	HandleDamage(OtherActor, Hit);
	HandleImpactEffects(Hit);
	FinishHit(Hit);
}

void AProjectile::HandleDamage(AActor* OtherActor, const FHitResult& Hit)
{
	// 普通 Projectile 如果以后要做直接伤害，可以写这里
}

void AProjectile::HandleImpactEffects(const FHitResult& Hit)
{
	const FVector ImpactPoint = Hit.ImpactPoint.IsNearlyZero()
		? GetActorLocation()
		: FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);

	const FVector ImpactNormal = Hit.ImpactNormal.IsNearlyZero()
		? -GetActorForwardVector()
		: FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, Hit.ImpactNormal.Z);

	//关于武器的获取，甚至可以在生成子弹的时候设置，也就是在子弹头文件定义一个武器变量，生成时顺便塞一个
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner()))
	{
		if (ABlasterWeapon* OwnerWeapon = BlasterCharacter->Combat->GetEquippedWeapon())
		{
			OwnerWeapon->MulticastImpactEffects(
			FVector_NetQuantize(ImpactPoint),
			FVector_NetQuantizeNormal(ImpactNormal));
		}
	}
}

void AProjectile::FinishHit(const FHitResult& Hit)
{
	Destroy();
}

void AProjectile::DisableProjectileAfterHit()
{
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

