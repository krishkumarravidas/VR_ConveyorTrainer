#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XRHandInteractor.h"
#include "TrainerStepManager.generated.h"

class ATrainerPanel;
class ATrainerButton;
class AConveyorPart;
class AConveyorFlow;

UENUM(BlueprintType)
enum class ETrainerStep : uint8
{
	Welcome,
	WorkOrder,
	Checklist,
	IdentifyParts,
	Inspect,
	DiagnosePart,
	DiagnoseResponse,
	Complete
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTrainerStepChanged, ETrainerStep, NewStep);

/**
 * Drives the 5-step inspection journey from the brief:
 * work order -> simulated stop/isolate checklist -> identify 3 parts -> inspect the fault -> diagnose + respond -> summary.
 * Wire the panels, buttons and parts in the Details panel (the level script does this for L_ConveyorTrainer).
 */
UCLASS(Blueprintable)
class VR_CONVEYORTRAINER_API ATrainerStepManager : public AActor
{
	GENERATED_BODY()

public:
	ATrainerStepManager();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<ATrainerPanel> InstructionPanel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<ATrainerPanel> WorkOrderPanel;

	/** Main button: Start / Accept / Continue / Restart. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<ATrainerButton> ActionButton;

	/** Three answer buttons, relabelled per step (checklist confirmations, diagnosis choices). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TArray<TObjectPtr<ATrainerButton>> ChoiceButtons;

	/** Belt, idler roller, motor housing. Exactly one should have bIsFaulty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TArray<TObjectPtr<AConveyorPart>> Parts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<AXRHandInteractor> Interactor;

	/** Boxes on the inspected conveyor. Runs until the trainee ticks "machine stopped". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<AConveyorFlow> LineFlow;

	/** Pick a new random damaged part (belt / roller / motor housing) every round. Off = keep the level's choice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	bool bRandomFault = true;

	/** Playtest build: live stats board, R / N hotkeys, reset button and a CSV log of every run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer|Playtest")
	bool bPlaytestMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer|Playtest")
	TObjectPtr<ATrainerPanel> PlaytestPanel;

	/** Optional poke button that restarts the run (playtest level). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer|Playtest")
	TObjectPtr<ATrainerButton> ResetButton;

	/** Acceptance target from the brief. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer|Playtest")
	float TargetSeconds = 240.f;

	UPROPERTY(BlueprintAssignable, Category = "Trainer")
	FTrainerStepChanged OnStepChanged;

	UFUNCTION(BlueprintPure, Category = "Trainer")
	ETrainerStep GetCurrentStep() const { return Step; }

	UFUNCTION(BlueprintCallable, Category = "Trainer")
	void GoToStep(ETrainerStep NewStep);

	UFUNCTION(BlueprintCallable, Category = "Trainer")
	void RestartTraining();

	/** Playtest helper: mark the current step done and move on. */
	UFUNCTION(BlueprintCallable, Category = "Trainer|Playtest")
	void SkipStep();

private:
	UFUNCTION()
	void HandleButton(ATrainerButton* Button);

	UFUNCTION()
	void HandlePart(AConveyorPart* Part);

	UFUNCTION()
	void HandleInputModeChanged(ETrainerInputMode NewMode);

	void ShowChoices(const TArray<FString>& Labels, bool bToggle);
	void ShowAction(const FString& Label);
	void SetPartsSelectable(bool bSelectable);
	void SetFeedback(const FString& Text, const FLinearColor& Color);
	void RefreshPanels();
	FString HowToLine() const;
	FString PartLabel(int32 Index) const;
	int32 FaultyIndex() const;
	int32 ChoiceIndex(ATrainerButton* Button) const;

	void UpdateLineFlow();
	void PickFault();
	int32 LastFaultIndex = INDEX_NONE;
	int32 ForcedFaultIndex = INDEX_NONE;
	void PlaytestTick();
	void RefreshPlaytestPanel();
	void LogRun();
	static FString StepName(ETrainerStep S);
	FString InputName() const;
	FTimerHandle PlaytestTimer;
	int32 PlaytestTicks = 0;
	bool bRWasDown = false;
	bool bNWasDown = false;
	int32 RunCount = 0;
	FString LastRunSummary;
	double StepEnterTime = 0.0;
	TMap<ETrainerStep, double> StepSeconds;

	/** Dev only: -TrainerShots walks through the steps and takes a screenshot of each, then quits. */
	void RunScreenshotTour();
	void TourTick();
	FTimerHandle TourTimer;
	int32 TourIndex = 0;

	ETrainerStep Step = ETrainerStep::Welcome;

	bool bStopped = false;
	bool bIsolated = false;
	TSet<int32> Identified;
	bool bFaultFound = false;
	bool bDiagnosed = false;
	bool bResponded = false;
	int32 Retries = 0;
	double StartTime = 0.0;
	double EndTime = 0.0;

	FString FeedbackText;
	FLinearColor FeedbackColor;
};
