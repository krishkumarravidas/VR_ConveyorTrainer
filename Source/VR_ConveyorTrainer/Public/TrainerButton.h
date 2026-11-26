#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainerInteractable.h"
#include "TrainerButton.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;
class ATrainerButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTrainerButtonPressed, ATrainerButton*, Button);

/**
 * Industrial push button: a coloured cap in a dark enclosure with an engraved-style label strip.
 * Poke it with a fingertip (or controller tip). On desktop, aim and left-click.
 * The face points along the actor's +X axis.
 */
UCLASS(Blueprintable)
class VR_CONVEYORTRAINER_API ATrainerButton : public AActor, public ITrainerInteractable
{
	GENERATED_BODY()

public:
	ATrainerButton();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	FString Label = TEXT("Button");

	/** Width x height in cm. Keep at least 6 x 6 so a fingertip hits it easily. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	FVector2D Size = FVector2D(30.f, 7.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	float TextSize = 2.4f;

	/** Cap colour when idle (green = go / start, yellow = choice, red = reset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	FLinearColor CapColor = FLinearColor(0.95f, 0.62f, 0.0f);

	/** Toggle buttons stay green once pressed (used for checklist confirmations). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	bool bIsToggle = false;

	UPROPERTY(BlueprintAssignable, Category = "Button")
	FTrainerButtonPressed OnPressed;

	UFUNCTION(BlueprintCallable, Category = "Button")
	void SetLabel(const FString& InLabel);

	/** Disabled buttons are hidden and ignore input. */
	UFUNCTION(BlueprintCallable, Category = "Button")
	void SetButtonEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Button")
	bool IsButtonEnabled() const { return bEnabledState; }

	UFUNCTION(BlueprintCallable, Category = "Button")
	void SetToggled(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Button")
	bool IsToggled() const { return bToggled; }

	// ITrainerInteractable
	virtual bool CanInteract_Implementation() const override;
	virtual void SetHovered_Implementation(bool bHovered) override;
	virtual void Interact_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Button")
	TObjectPtr<UBoxComponent> Hitbox;

	UPROPERTY(VisibleAnywhere, Category = "Button")
	TObjectPtr<UStaticMeshComponent> Plate;

	UPROPERTY(VisibleAnywhere, Category = "Button")
	TObjectPtr<UStaticMeshComponent> Bezel;

	UPROPERTY(VisibleAnywhere, Category = "Button")
	TObjectPtr<UStaticMeshComponent> Cap;

	UPROPERTY(VisibleAnywhere, Category = "Button")
	TObjectPtr<UStaticMeshComponent> LabelStrip;

	UPROPERTY(VisibleAnywhere, Category = "Button")
	TObjectPtr<UTextRenderComponent> LabelText;

private:
	void InitVisuals();
	void Layout();
	void UpdateColor();
	void EndPressAnim();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlateMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CapMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StripMID;

	bool bEnabledState = true;
	bool bHoveredState = false;
	bool bToggled = false;
	bool bPressAnim = false;
	double LastPressTime = -10.0;
	FTimerHandle PressTimer;
};
