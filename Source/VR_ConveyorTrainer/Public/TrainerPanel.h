#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainerPanel.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class ETrainerPanelStyle : uint8
{
	Whiteboard,   // framed off-white board with masking tape corners
	Clipboard,    // brown hardboard, paper sheet, metal clip
	Sign          // safety-yellow sign, black text
};

/**
 * In-world info board (instructions, work order, signs). Looks like a physical shop-floor board, not a floating HUD.
 * The readable face points along the actor's +X axis, so rotate the actor to face the player.
 */
UCLASS(Blueprintable)
class VR_CONVEYORTRAINER_API ATrainerPanel : public AActor
{
	GENERATED_BODY()

public:
	ATrainerPanel();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	ETrainerPanelStyle Style = ETrainerPanelStyle::Whiteboard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	float Width = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	float Height = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	float TitleSize = 4.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	float BodySize = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	float FooterSize = 2.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	FString Title = TEXT("Title");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (MultiLine = true))
	FString Body = TEXT("Body text");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (MultiLine = true))
	FString Footer;

	UFUNCTION(BlueprintCallable, Category = "Panel")
	void SetContent(const FString& InTitle, const FString& InBody, const FString& InFooter);

	/** One short coloured line between body and footer (green = correct, orange = try again). Empty hides it. */
	UFUNCTION(BlueprintCallable, Category = "Panel")
	void SetFeedback(const FString& InText, FLinearColor Color);

	/** Colour of the status strip on the left edge (yellow = working, green = done, orange = try again). */
	UFUNCTION(BlueprintCallable, Category = "Panel")
	void SetAccentColor(FLinearColor Color);

	/** Current is 1-based. Total 0 hides the "STEP n/N" tag. */
	UFUNCTION(BlueprintCallable, Category = "Panel")
	void SetProgress(int32 Current, int32 Total);

	/** Dimmed panels are not the focus of the current step (text fades). */
	UFUNCTION(BlueprintCallable, Category = "Panel")
	void SetDimmed(bool bInDimmed);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> Backing;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> Sheet;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> StatusStrip;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> TitleRule;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> TapeA;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> TapeB;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UStaticMeshComponent> Clip;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UTextRenderComponent> TitleText;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UTextRenderComponent> StepText;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UTextRenderComponent> BodyText;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UTextRenderComponent> FeedbackText;

	UPROPERTY(VisibleAnywhere, Category = "Panel")
	TObjectPtr<UTextRenderComponent> FooterText;

private:
	void InitVisuals();
	void Layout();
	void ApplyColors();
	int32 CharsPerLine(float Size, bool bCaps = false) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BackingMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SheetMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StripMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InkMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TapeMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ClipMID;

	FLinearColor AccentColor;
	int32 ProgressCurrent = 0;
	int32 ProgressTotal = 0;
	bool bDimmed = false;
};
