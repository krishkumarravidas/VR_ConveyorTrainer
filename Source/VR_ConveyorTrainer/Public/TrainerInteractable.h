#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TrainerInteractable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UTrainerInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Anything the trainee can point at / poke: conveyor parts and 3D buttons. Same path for hands, controllers and desktop. */
class VR_CONVEYORTRAINER_API ITrainerInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Trainer")
	bool CanInteract() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Trainer")
	void SetHovered(bool bHovered);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Trainer")
	void Interact();
};
