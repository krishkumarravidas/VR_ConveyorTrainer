#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainerInteractable.h"
#include "ConveyorPart.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UBoxComponent;
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AConveyorPart;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConveyorPartSelected, AConveyorPart*, Part);

UENUM(BlueprintType)
enum class EPartLabelMode : uint8
{
	Hidden,
	Marker,          // "?" — find me
	NameAndRole,     // name + what it does
	NameOnly,
	InspectNote      // name + condition found on inspection
};

/**
 * A conveyor component the trainee touches (belt, idler roller, motor housing).
 * A touch zone slightly bigger than the mesh makes it easy to poke. Labels look like hanging inspection tags.
 */
UCLASS(Blueprintable)
class VR_CONVEYORTRAINER_API AConveyorPart : public AActor, public ITrainerInteractable
{
	GENERATED_BODY()

public:
	AConveyorPart();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	TObjectPtr<UStaticMesh> PartMesh;

	/** Optional per-slot materials (e.g. M_Damaged for the faulty roller). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	TArray<TObjectPtr<UMaterialInterface>> MaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	/** Name shown on the label, e.g. "Idler roller". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	FString PartName = TEXT("Part");

	/** What the part does (shown in step 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor", meta = (MultiLine = true))
	FString Description;

	/** What the trainee sees when inspecting it (step 4). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor", meta = (MultiLine = true))
	FString InspectNote = TEXT("No visible damage.");

	/** True for the one intentionally damaged part. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	bool bIsFaulty = false;

	/** Extra size (cm, each side) around the mesh that still counts as touching it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	float TouchMargin = 3.f;

	/** Label position relative to the actor, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conveyor")
	FVector LabelOffset = FVector(0.f, 0.f, 60.f);

	UPROPERTY(BlueprintAssignable, Category = "Conveyor")
	FConveyorPartSelected OnSelected;

	UFUNCTION(BlueprintCallable, Category = "Conveyor")
	void SetSelectable(bool bInSelectable);

	UFUNCTION(BlueprintCallable, Category = "Conveyor")
	void SetLabelMode(EPartLabelMode Mode);

	/** Forces the highlight on (in addition to hover highlight). */
	UFUNCTION(BlueprintCallable, Category = "Conveyor")
	void SetHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintPure, Category = "Conveyor")
	bool IsHighlighted() const { return bForcedHighlight || bHoveredState; }

	/** Blueprint hook fired when the trainee selects this part. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Conveyor")
	void OnPartSelected();

	// ITrainerInteractable
	virtual bool CanInteract_Implementation() const override;
	virtual void SetHovered_Implementation(bool bHovered) override;
	virtual void Interact_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Conveyor")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Poke / aim target. Blocks the Visibility channel only. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Conveyor")
	TObjectPtr<UBoxComponent> TouchZone;

	UPROPERTY(VisibleAnywhere, Category = "Conveyor")
	TObjectPtr<USceneComponent> LabelRoot;

	/** Thin string from the tag to the part. */
	UPROPERTY(VisibleAnywhere, Category = "Conveyor")
	TObjectPtr<UStaticMeshComponent> LeaderLine;

	UPROPERTY(VisibleAnywhere, Category = "Conveyor")
	TObjectPtr<UStaticMeshComponent> LabelBackground;

	UPROPERTY(VisibleAnywhere, Category = "Conveyor")
	TObjectPtr<UTextRenderComponent> LabelText;

private:
	void InitVisuals();
	void ApplyMaterials();
	void RefreshLabel();
	void UpdateTouchZone();
	void UpdateLeader();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LabelBgMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FallbackHighlightMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LeaderMID;

	float LabelHeight = 0.f;

	bool bSelectable = false;
	bool bHoveredState = false;
	bool bForcedHighlight = false;
	EPartLabelMode LabelMode = EPartLabelMode::Hidden;
};
