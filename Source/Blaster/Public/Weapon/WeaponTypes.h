#pragma once


#define TRACE_LENGTH 8000.f

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE   251
#define CUSTOM_DEPTH_TAN    252

UENUM(BlueprintType)
namespace EWeaponType 
{
	enum Type
	{
		EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
		EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
		EWT_Pistol UMETA(DisplayName = "Pistol"),
		EWT_SubmachineGun UMETA(DisplayName = "Submachine"),
		EWT_Shotgun UMETA(DisplayName = "Shotgun"),
		EWT_SniperRifle UMETA(DisplayName = "SniperRifle"),
		
		EWT_MAX UMETA(DisplayName = "DefaultMAX")
	};
}
