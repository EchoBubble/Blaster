#pragma once

UENUM(BlueprintType)
namespace ETurningInPlace
{
	enum Type
	{
		ETIP_Left UMETA(DisplayName = "Turning Left"),
        ETIP_Right UMETA(DisplayName = "Turning Right"),
        ETIP_NotTurning UMETA(DisplayName = "Not Turning")
	};
	
}

