// Fill out your copyright notice in the Description page of Project Settings.


#include "Blaster/Public/Anim/BlasterAnimInstance.h"

#include "Blaster/Public/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/BlasterWeapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if (BlasterCharacter == nullptr) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = !BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero();
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon =  BlasterCharacter->GetEquippedWeapon();
	bIsCrouch = BlasterCharacter->bIsCrouched;
	bAiming = BlasterCharacter->IsAiming();
	bFireButtonPressed = BlasterCharacter->IsFiring();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();//需注意，该值是根据全局坐标计算的，不是局部坐标
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());//和 GetBaseAimRotation 一样都是全局的，因此可以计算他们的插值
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);//把结果规范到 -180 到 180 之间
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 15.f);
	YawOffset = DeltaRotation.Yaw;
	
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;//算每秒旋转多少度
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
	
	FRotator Rotation = UKismetMathLibrary::NormalizedDeltaRotator(BlasterCharacter->GetControlRotation(), BlasterCharacter->GetActorRotation());
	AimOffsetYaw = BlasterCharacter->GetAO_Yaw();
	AimOffsetPitch = BlasterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->WeaponMesh && BlasterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->WeaponMesh->GetSocketTransform(FName("LeftHandSocket"), RTS_World);//拿插槽位置
		FVector OutPosition;
		FRotator OutRotation;
		//获取 LeftHandSocket 相对于右手 hand_r 的位置和旋转
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
		
		if (EquippedWeapon)
		{
			EquippedWeaponType = EquippedWeapon->GetWeaponType();

			bUsePistolAnimations = EquippedWeaponType == EWeaponType::EWT_Pistol;
			bUseRifleAnimations = EquippedWeaponType == EWeaponType::EWT_AssaultRifle;
			bUseRocketAnimations = EquippedWeaponType == EWeaponType::EWT_RocketLauncher;
			bUseShotgunAnimations = EquippedWeaponType == EWeaponType::EWT_Shotgun;
		}
		else
		{
			bUsePistolAnimations = false;
			bUseRifleAnimations = false;
			bUseRocketAnimations = false;
			bUseShotgunAnimations = false;
		}
		
		//暂时不用这个，有问题
		/*if (BlasterCharacter->IsLocallyControlled())
		{
			FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), RTS_World
			);

			RightHandRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() +
				(RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget())
			);

			FTransform MuzzleTipTransform = EquippedWeapon->WeaponMesh->GetSocketTransform(FName("MuzzleFlash"),ERelativeTransformSpace::RTS_World);

			FVector MuzzleX = FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X);//获取枪口 X 朝向
			DrawDebugLine(
			GetWorld(),
			MuzzleTipTransform.GetLocation(),
			MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f,
			FColor::Red);

			DrawDebugLine(
				GetWorld(),
				MuzzleTipTransform.GetLocation(),
				BlasterCharacter->GetHitTarget(),
				FColor::Orange);
		}*/
	}

	bUseFABRIK = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	
}


