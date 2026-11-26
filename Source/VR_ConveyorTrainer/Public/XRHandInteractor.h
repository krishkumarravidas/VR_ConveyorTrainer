#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "XRHandInteractor.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class ETrainerInputMode : uint8
{
	Desktop,
	Controllers,
	Hands
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTrainerInputModeChanged, ETrainerInputMode, NewMode);

/**
 * One place for all trainee input. Place one in the level. POKE ONLY: no pinch, no laser.
 *  - Hands (OpenXR hand tracking): joint spheres; touch buttons and parts with your index fingertip.
 *  - Controllers: a small tip ball in front of each controller; touch things with it.
 *  - Desktop (no headset, for playtesting): WASD to walk, hold right mouse to look, aim the dot and left-click
 *    to "touch" anything within arm's reach (DesktopReach).
 * Everything ends in ITrainerInteractable::Interact, so step logic does not care which input was used.
 */
UCLASS(Blueprintable)
class VR_CONVEYORTRAINER_API AXRHandInteractor : public AActor
{
	GENERATED_BODY()

public:
	AXRHandInteractor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Input")
	ETrainerInputMode GetInputMode() const { return InputMode; }

	UPROPERTY(BlueprintAssignable, Category = "Input")
	FTrainerInputModeChanged OnInputModeChanged;

	/** Fingertip touch radius for poking (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Hands")
	float PokeRadius = 1.2f;

	/** Things within this distance of a fingertip / controller tip light up as "about to touch" (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Hands")
	float HoverRadius = 8.f;

	/** How far in front of the controller grip the touch tip sits (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Controllers")
	float ControllerTipOffset = 6.f;

	/** Desktop: max distance from the eye to something you can click (simulated arm's reach plus a lean). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Desktop")
	float DesktopReach = 230.f;

	/** Desktop walk speed (WASD), cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Desktop")
	float DesktopWalkSpeed = 160.f;

	/** Desktop eye height used when no headset is connected (cm above the floor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Desktop")
	float DesktopEyeHeight = 165.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> HandJoints;

	/** Controller touch tips (hidden for hands). */
	UPROPERTY(VisibleAnywhere, Category = "Input")
	TArray<TObjectPtr<UStaticMeshComponent>> Tips;

	/** Desktop aim dot. */
	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<UStaticMeshComponent> AimDot;

private:
	struct FTipState
	{
		bool bActive = false;
		bool bHasLastTip = false;
		FVector LastTip = FVector::ZeroVector;
		TWeakObjectPtr<AActor> HoverTarget;
		TSet<TWeakObjectPtr<AActor>> Poking;
	};

	void SetInputMode(ETrainerInputMode NewMode);
	bool UpdateHand(int32 Index);
	bool UpdateController(int32 Index);
	void UpdateDesktop(class APlayerController* PC, float DeltaSeconds);
	void UpdatePoke(int32 Index, const FVector& Tip);
	void HideHand(int32 Index);
	void FixDesktopCamera(APawn* Pawn);
	static bool IsInteractable(AActor* Actor);

	FTipState Tips_[2];
	TWeakObjectPtr<AActor> DesktopHover;
	bool bMouseWasDown = false;
	TSet<TWeakObjectPtr<AActor>> HoveredLastFrame;
	ETrainerInputMode InputMode = ETrainerInputMode::Desktop;
	bool bDesktopCameraFixed = false;
	float DesktopFloorZ = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HandMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TipMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AimMID;
};
