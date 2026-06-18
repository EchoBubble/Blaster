// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"


ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = true;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>("CasingMesh");
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);//由于开启了 Hit 事件，这里得开启，否则不会触发 Hit 事件

	CasingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CasingMesh->SetCollisionObjectType(ECC_PhysicsBody);

	CasingMesh->SetCollisionResponseToAllChannels(ECR_Block);
	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	CasingMesh->SetGenerateOverlapEvents(true);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	CasingMesh->AddImpulse(-GetActorForwardVector() * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit) return;
	bHasHit = true;

	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ShellSound,
			GetActorLocation()
		);
	}
	
	SetLifeSpan(3.f);
}



