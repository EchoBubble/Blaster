#pragma once

UENUM(BlueprintType)
namespace ECombatState
{
	enum Type
	{
		ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
		ECS_Reloading UMETA(DisplayName = "Reloading"),

		ECS_MAX UMETA(DisplayName = "DefaultMAX")
	};
}