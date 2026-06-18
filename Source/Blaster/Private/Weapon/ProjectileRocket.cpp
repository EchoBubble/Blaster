// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/Blaster.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/RocketMovementComponent.h"


AProjectileRocket::AProjectileRocket()
{
	/*RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);*/

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->InitialSpeed = 3200.f;
	RocketMovementComponent->MaxSpeed = 3200.f;
	RocketMovementComponent->ProjectileGravityScale = 0.7f;
	RocketMovementComponent->bInitialVelocityInLocalSpace = true;//Velocity 按子弹自身方向理解
	RocketMovementComponent->Velocity = FVector::ForwardVector;//朝自己的 X 轴方向飞
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false);
	}
}

void AProjectileRocket::HandleDamage(AActor* OtherActor, const FHitResult& Hit)
{
	if (HasAuthority())
	{
		APawn* FiringPawn = GetInstigator();
		if (FiringPawn)
		{
			AController* FiringController = FiringPawn->GetController<AController>();
			if (FiringController)
			{
				UGameplayStatics::ApplyRadialDamageWithFalloff(
					this,
					Damage,
					10.f,
					GetActorLocation(),
					150.f,
					300.f,
					1.f,
					UDamageType::StaticClass(),
					TArray<AActor*>(),
					this,
					FiringController
					);
			}
		}
	}
}

void AProjectileRocket::FinishHit(const FHitResult& Hit)
{
	MulticastHandleRocketHitEffects();
	
	SetLifeSpan(DestroyDelayAfterHit);
}

void AProjectileRocket::MulticastHandleRocketHitEffects_Implementation()
{
	if (TracerComponent)
    {
    	TracerComponent->Deactivate();
    	TracerComponent->DestroyComponent();
    	TracerComponent = nullptr;
    }
    
    if (TrailSystemComponent)
    {
    	TrailSystemComponent->Deactivate();
    }
	
	DisableProjectileAfterHit();
	
}

/*
void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority())
	{
		APawn* FiringPawn = GetInstigator();
		if (FiringPawn)
		{
			AController* FiringController = FiringPawn->GetController<AController>();
			if (FiringController)
			{
				UGameplayStatics::ApplyRadialDamageWithFalloff(
					this,
					Damage,
					10.f,
					GetActorLocation(),
					150.f,
					300.f,
					1.f,
					UDamageType::StaticClass(),
					TArray<AActor*>(),
					this,
					FiringController
					);
			}
		}
	}
	
}
*/


