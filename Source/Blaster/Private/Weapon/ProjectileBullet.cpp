// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

/*void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterCharacter)
	{
		if (AController* Controller = BlasterCharacter->GetController())
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, Controller, this, UDamageType::StaticClass());
		}
	}
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}*/

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(FName("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;//子弹往哪飞，就朝向哪里，而不是永远平视
	ProjectileMovementComponent->InitialSpeed = 7000.f;
	ProjectileMovementComponent->MaxSpeed = 7000.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.1f;
	ProjectileMovementComponent->bInitialVelocityInLocalSpace = true;//Velocity 按子弹自身方向理解
	ProjectileMovementComponent->Velocity = FVector::ForwardVector;//朝自己的 X 轴方向飞
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileBullet::HandleDamage(AActor* OtherActor, const FHitResult& Hit)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterCharacter)
	{
		if (AController* Controller = BlasterCharacter->GetController())
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, Controller, this, UDamageType::StaticClass());
		}
	}
}
