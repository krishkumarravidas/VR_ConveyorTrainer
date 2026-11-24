#include "TrainerStepManager.h"
#include "ConveyorPart.h"
#include "ConveyorFlow.h"
#include "TrainerButton.h"
#include "TrainerPanel.h"
#include "TrainerStyle.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static const TCHAR* SimNote = TEXT("TRAINING SIM - not a real lockout/tagout procedure.");

static FString CheckBox(bool b) { return b ? TEXT("[x] ") : TEXT("[ ] "); }

ATrainerStepManager::ATrainerStepManager()
{
	PrimaryActorTick.bCanEverTick = false;
	FeedbackColor = TrainerStyle::Ink;
}

void ATrainerStepManager::BeginPlay()
{
	Super::BeginPlay();

	// Fall back to finding things in the level if they were not wired.
	if (!Interactor)
	{
		for (TActorIterator<AXRHandInteractor> It(GetWorld()); It; ++It) { Interactor = *It; break; }
	}
	if (Parts.Num() == 0)
	{
		for (TActorIterator<AConveyorPart> It(GetWorld()); It; ++It) { Parts.Add(*It); }
	}

	if (ActionButton) { ActionButton->OnPressed.AddDynamic(this, &ATrainerStepManager::HandleButton); }
	for (ATrainerButton* B : ChoiceButtons)
	{
		if (B) { B->OnPressed.AddDynamic(this, &ATrainerStepManager::HandleButton); }
	}
	for (AConveyorPart* P : Parts)
	{
		if (P) { P->OnSelected.AddDynamic(this, &ATrainerStepManager::HandlePart); }
	}
	if (Interactor) { Interactor->OnInputModeChanged.AddDynamic(this, &ATrainerStepManager::HandleInputModeChanged); }
	if (ResetButton) { ResetButton->OnPressed.AddDynamic(this, &ATrainerStepManager::HandleButton); }

	RestartTraining();

	if (bPlaytestMode)
	{
		GetWorldTimerManager().SetTimer(PlaytestTimer, this, &ATrainerStepManager::PlaytestTick, 0.05f, true);
		UE_LOG(LogTemp, Log, TEXT("Trainer: playtest mode on. Runs log to %s"), *(FPaths::ProjectSavedDir() / TEXT("Playtest/playtest_runs.csv")));
	}

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("TrainerShots")))
	{
		RunScreenshotTour();
	}
#endif
}

void ATrainerStepManager::RunScreenshotTour()
{
	TourIndex = 0;
	// Give shaders and the pawn time to settle before the first shot.
	GetWorldTimerManager().SetTimer(TourTimer, this, &ATrainerStepManager::TourTick, 2.0f, true, 20.0f);
}

void ATrainerStepManager::TourTick()
{
	auto Shot = [this]() { FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("TrainerTour_%02d.png"), TourIndex), false, false); };
	auto View = [this](float Yaw, float Pitch)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (UCameraComponent* Cam = PC && PC->GetPawn() ? PC->GetPawn()->FindComponentByClass<UCameraComponent>() : nullptr)
		{
			Cam->SetRelativeRotation(FRotator(Pitch, Yaw, 0.f));
		}
	};
	// Matches the layout in Scripts/build_conveyor_level.py (player at the origin facing +X).
	const float StationYaw = 62.f, StationPitch = -38.f;
	const float BoardYaw = 11.f, BoardPitch = 2.f;
	const float ClipYaw = -83.f, ClipPitch = -26.f;
	switch (TourIndex++)
	{
	case 0: View(18.f, -12.f); break;
	case 1: Shot(); break;                                                // factory overview, belt running
	case 2: View(StationYaw, StationPitch); break;
	case 3: Shot(); break;                                                // welcome: control box
	case 4: View(BoardYaw, BoardPitch); break;
	case 5: Shot(); break;                                                // welcome: instruction board
	case 6: GoToStep(ETrainerStep::WorkOrder); View(ClipYaw, ClipPitch); break;
	case 7: Shot(); break;
	case 8: GoToStep(ETrainerStep::Checklist); if (ChoiceButtons.Num() > 0 && ChoiceButtons[0]) ITrainerInteractable::Execute_Interact(ChoiceButtons[0]); View(StationYaw, StationPitch); break;
	case 9: Shot(); break;
	case 10: GoToStep(ETrainerStep::IdentifyParts); if (Parts.Num() > 0) HandlePart(Parts[0]); View(4.f, -28.f); break;
	case 11: Shot(); break;
	case 12: for (int32 i = 0; i < Parts.Num(); ++i) Identified.Add(i); GoToStep(ETrainerStep::Inspect); HandlePart(Parts[FMath::Max(0, FaultyIndex())]); View(6.f, -44.f); break;
	case 13: Shot(); break;
	case 14: GoToStep(ETrainerStep::DiagnoseResponse); View(StationYaw, StationPitch); break;
	case 15: Shot(); break;
	case 16: bStopped = bIsolated = bFaultFound = bDiagnosed = bResponded = true; GoToStep(ETrainerStep::Complete); View(BoardYaw, BoardPitch); break;
	case 17: Shot(); break;
	case 18: View(ClipYaw, ClipPitch); break;
	case 19: Shot(); break;
	case 20: View(-100.f, -8.f); break;
	case 21: Shot(); break;                                               // left side (playtest board if present)
	default:
		GetWorldTimerManager().ClearTimer(TourTimer);
		GEngine->Exec(GetWorld(), TEXT("quit"));
		break;
	}
}

void ATrainerStepManager::RestartTraining()
{
	bStopped = bIsolated = bFaultFound = bDiagnosed = bResponded = false;
	Identified.Reset();
	Retries = 0;
	StartTime = EndTime = 0.0;
	StepEnterTime = 0.0;
	StepSeconds.Reset();
	for (AConveyorPart* P : Parts)
	{
		if (!P) continue;
		P->SetHighlighted(false);
		P->SetLabelMode(EPartLabelMode::Hidden);
	}
	GoToStep(ETrainerStep::Welcome);
}

void ATrainerStepManager::GoToStep(ETrainerStep NewStep)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (StepEnterTime > 0.0) { StepSeconds.FindOrAdd(Step) += Now - StepEnterTime; }
	StepEnterTime = Now;

	Step = NewStep;
	SetFeedback(FString(), TrainerStyle::Ink);

	switch (Step)
	{
	case ETrainerStep::Welcome:
		ShowChoices({}, false);
		ShowAction(TEXT("Start"));
		SetPartsSelectable(false);
		break;

	case ETrainerStep::WorkOrder:
		StartTime = Now;
		ShowChoices({}, false);
		ShowAction(TEXT("Accept"));
		SetPartsSelectable(false);
		break;

	case ETrainerStep::Checklist:
		ShowChoices({ TEXT("Stop machine (sim)"), TEXT("Isolate power (sim)") }, true);
		ShowAction(FString());
		SetPartsSelectable(false);
		break;

	case ETrainerStep::IdentifyParts:
		ShowChoices({}, false);
		ShowAction(FString());
		SetPartsSelectable(true);
		for (int32 i = 0; i < Parts.Num(); ++i)
		{
			if (Parts[i]) { Parts[i]->SetLabelMode(Identified.Contains(i) ? EPartLabelMode::NameAndRole : EPartLabelMode::Marker); }
		}
		break;

	case ETrainerStep::Inspect:
		ShowChoices({}, false);
		ShowAction(FString());
		SetPartsSelectable(true);
		for (AConveyorPart* P : Parts) { if (P) P->SetLabelMode(EPartLabelMode::NameOnly); }
		break;

	case ETrainerStep::DiagnosePart:
	{
		SetPartsSelectable(false);
		TArray<FString> Names;
		for (int32 i = 0; i < Parts.Num(); ++i) { Names.Add(PartLabel(i)); }
		ShowChoices(Names, false);
		ShowAction(FString());
		break;
	}

	case ETrainerStep::DiagnoseResponse:
		SetPartsSelectable(false);
		ShowChoices({
			TEXT("Restart belt to test it"),
			TEXT("Report to maintenance"),
			TEXT("Fix the roller myself") }, false);
		ShowAction(FString());
		break;

	case ETrainerStep::Complete:
		EndTime = Now;
		SetPartsSelectable(false);
		ShowChoices({}, false);
		ShowAction(TEXT("Restart"));
		for (AConveyorPart* P : Parts)
		{
			if (!P) continue;
			P->SetLabelMode(P->bIsFaulty ? EPartLabelMode::InspectNote : EPartLabelMode::NameOnly);
			P->SetHighlighted(false);
		}
		if (bPlaytestMode) { LogRun(); }
		break;
	}

	UpdateLineFlow();
	RefreshPanels();
	OnStepChanged.Broadcast(Step);
}

void ATrainerStepManager::SkipStep()
{
	switch (Step)
	{
	case ETrainerStep::Welcome:          GoToStep(ETrainerStep::WorkOrder); break;
	case ETrainerStep::WorkOrder:        GoToStep(ETrainerStep::Checklist); break;
	case ETrainerStep::Checklist:        bStopped = bIsolated = true; GoToStep(ETrainerStep::IdentifyParts); break;
	case ETrainerStep::IdentifyParts:    for (int32 i = 0; i < Parts.Num(); ++i) Identified.Add(i); GoToStep(ETrainerStep::Inspect); break;
	case ETrainerStep::Inspect:          bFaultFound = true; GoToStep(ETrainerStep::DiagnosePart); break;
	case ETrainerStep::DiagnosePart:     bDiagnosed = true; GoToStep(ETrainerStep::DiagnoseResponse); break;
	case ETrainerStep::DiagnoseResponse: bResponded = true; GoToStep(ETrainerStep::Complete); break;
	case ETrainerStep::Complete:         RestartTraining(); break;
	}
}

void ATrainerStepManager::UpdateLineFlow()
{
	if (LineFlow) { LineFlow->SetRunning(!bStopped); }
}

void ATrainerStepManager::HandleButton(ATrainerButton* Button)
{
	if (Button && Button == ResetButton)
	{
		RestartTraining();
		return;
	}

	const bool bAction = Button == ActionButton;
	const int32 Choice = ChoiceIndex(Button);

	switch (Step)
	{
	case ETrainerStep::Welcome:
		if (bAction) GoToStep(ETrainerStep::WorkOrder);
		return;

	case ETrainerStep::WorkOrder:
		if (bAction) GoToStep(ETrainerStep::Checklist);
		return;

	case ETrainerStep::Checklist:
		if (bAction && bStopped && bIsolated) { GoToStep(ETrainerStep::IdentifyParts); return; }
		if (Choice == 0) bStopped = Button->IsToggled();
		if (Choice == 1) bIsolated = Button->IsToggled();
		UpdateLineFlow();
		if (bStopped && bIsolated)
		{
			SetFeedback(TEXT("OK - stopped and isolated (simulated)."), TrainerStyle::Good);
			ShowAction(TEXT("Continue"));
		}
		else
		{
			SetFeedback(bStopped ? TEXT("Belt stopping. Now isolate power.") : FString(), TrainerStyle::Ink);
			ShowAction(FString());
		}
		RefreshPanels();
		return;

	case ETrainerStep::IdentifyParts:
		if (bAction && Identified.Num() >= Parts.Num()) GoToStep(ETrainerStep::Inspect);
		return;

	case ETrainerStep::Inspect:
		if (bAction && bFaultFound) GoToStep(ETrainerStep::DiagnosePart);
		return;

	case ETrainerStep::DiagnosePart:
		if (Choice < 0) return;
		if (Choice == FaultyIndex())
		{
			bDiagnosed = true;
			GoToStep(ETrainerStep::DiagnoseResponse);
			SetFeedback(FString::Printf(TEXT("Right - the %s is damaged."), *PartLabel(Choice).ToLower()), TrainerStyle::Good);
			RefreshPanels();
		}
		else
		{
			++Retries;
			SetFeedback(TEXT("No. Which part looked damaged?"), TrainerStyle::Warn);
			RefreshPanels();
		}
		return;

	case ETrainerStep::DiagnoseResponse:
		if (Choice == 1)
		{
			bResponded = true;
			GoToStep(ETrainerStep::Complete);
		}
		else if (Choice == 0)
		{
			++Retries;
			SetFeedback(TEXT("No - never restart with a known fault."), TrainerStyle::Warn);
			RefreshPanels();
		}
		else if (Choice == 2)
		{
			++Retries;
			SetFeedback(TEXT("No - repairs are for qualified maintenance."), TrainerStyle::Warn);
			RefreshPanels();
		}
		return;

	case ETrainerStep::Complete:
		if (bAction) RestartTraining();
		return;
	}
}

void ATrainerStepManager::HandlePart(AConveyorPart* Part)
{
	const int32 Index = Parts.IndexOfByKey(Part);
	if (Index == INDEX_NONE) return;

	if (Step == ETrainerStep::IdentifyParts)
	{
		Identified.Add(Index);
		Part->SetLabelMode(EPartLabelMode::NameAndRole);
		SetFeedback(FString::Printf(TEXT("%s - %s"), *Part->PartName.ToUpper(), *Part->Description), TrainerStyle::Good);
		if (Identified.Num() >= Parts.Num())
		{
			ShowAction(TEXT("Continue"));
		}
		RefreshPanels();
	}
	else if (Step == ETrainerStep::Inspect)
	{
		Part->SetLabelMode(EPartLabelMode::InspectNote);
		if (Part->bIsFaulty)
		{
			bFaultFound = true;
			SetFeedback(FString::Printf(TEXT("Found it: %s"), *Part->InspectNote), TrainerStyle::Good);
			ShowAction(TEXT("Continue"));
		}
		else if (!bFaultFound)
		{
			SetFeedback(FString::Printf(TEXT("%s looks OK. Keep looking."), *Part->PartName), TrainerStyle::Warn);
		}
		RefreshPanels();
	}
}

void ATrainerStepManager::HandleInputModeChanged(ETrainerInputMode NewMode)
{
	RefreshPanels();
}

void ATrainerStepManager::ShowChoices(const TArray<FString>& Labels, bool bToggle)
{
	for (int32 i = 0; i < ChoiceButtons.Num(); ++i)
	{
		ATrainerButton* B = ChoiceButtons[i];
		if (!B) continue;
		const bool bShow = Labels.IsValidIndex(i);
		B->bIsToggle = bToggle;
		if (bShow) { B->SetLabel(Labels[i]); }
		B->SetToggled(false);
		B->SetButtonEnabled(bShow);
	}
}

void ATrainerStepManager::ShowAction(const FString& Label)
{
	if (!ActionButton) return;
	if (!Label.IsEmpty()) { ActionButton->SetLabel(Label); }
	ActionButton->SetButtonEnabled(!Label.IsEmpty());
}

void ATrainerStepManager::SetPartsSelectable(bool bSelectable)
{
	for (AConveyorPart* P : Parts)
	{
		if (P) P->SetSelectable(bSelectable);
	}
}

void ATrainerStepManager::SetFeedback(const FString& Text, const FLinearColor& Color)
{
	FeedbackText = Text;
	FeedbackColor = Color;
}

FString ATrainerStepManager::HowToLine() const
{
	switch (Interactor ? Interactor->GetInputMode() : ETrainerInputMode::Desktop)
	{
	case ETrainerInputMode::Hands:       return TEXT("HANDS: touch buttons and parts with your fingertip.");
	case ETrainerInputMode::Controllers: return TEXT("CONTROLLERS: touch things with the yellow ball on the controller tip.");
	default:                             return TEXT("DESKTOP: WASD walk, hold right mouse to look, left-click to touch (in reach).");
	}
}

FString ATrainerStepManager::PartLabel(int32 Index) const
{
	return Parts.IsValidIndex(Index) && Parts[Index] ? Parts[Index]->PartName : FString(TEXT("?"));
}

int32 ATrainerStepManager::FaultyIndex() const
{
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		if (Parts[i] && Parts[i]->bIsFaulty) return i;
	}
	return INDEX_NONE;
}

int32 ATrainerStepManager::ChoiceIndex(ATrainerButton* Button) const
{
	return ChoiceButtons.IndexOfByKey(Button);
}

void ATrainerStepManager::RefreshPanels()
{
	if (InstructionPanel)
	{
		FString Title, Body;
		int32 Progress = 0;

		switch (Step)
		{
		case ETrainerStep::Welcome:
			Title = TEXT("Conveyor inspection");
			Body = TEXT("Line 2 has a fault report.\nInspect it, find the fault, report it. About 3 min.\n\nPress the green START button on your right.");
			break;
		case ETrainerStep::WorkOrder:
			Progress = 1;
			Title = TEXT("Read the work order");
			Body = TEXT("Clipboard on your left.\nPress ACCEPT when you have read it.");
			break;
		case ETrainerStep::Checklist:
			Progress = 2;
			Title = TEXT("Make it safe");
			Body = bStopped ? TEXT("Belt stopped.\nNow press ISOLATE POWER.") : TEXT("The belt is still running.\nPress STOP MACHINE, then ISOLATE POWER.");
			break;
		case ETrainerStep::IdentifyParts:
		{
			Progress = 3;
			Title = TEXT("Find the parts");
			Body = TEXT("Touch each part with a ? tag:");
			for (int32 i = 0; i < Parts.Num(); ++i)
			{
				Body += TEXT("\n") + CheckBox(Identified.Contains(i)) + PartLabel(i);
			}
			break;
		}
		case ETrainerStep::Inspect:
			Progress = 4;
			Title = TEXT("Inspect");
			Body = TEXT("One part is damaged. Look closely - crouch to see under the belt.\nTouch the damaged part.");
			break;
		case ETrainerStep::DiagnosePart:
			Progress = 5;
			Title = TEXT("Diagnose");
			Body = TEXT("Which part is faulty?\nAnswer on the control box.");
			break;
		case ETrainerStep::DiagnoseResponse:
			Progress = 5;
			Title = TEXT("What next?");
			Body = TEXT("Pick the right action on the control box.");
			break;
		case ETrainerStep::Complete:
		{
			const int32 Secs = FMath::RoundToInt(EndTime - StartTime);
			Title = TEXT("Done");
			Body = FString::Printf(
				TEXT("Parts found: %d/%d\nFault: %s\nAction: reported to maintenance\nRetries: %d\nTime: %d:%02d"),
				Identified.Num(), Parts.Num(), *PartLabel(FaultyIndex()), Retries, Secs / 60, Secs % 60);
			if (FeedbackText.IsEmpty())
			{
				SetFeedback(TEXT("Logged. Fault handed to maintenance."), TrainerStyle::Good);
			}
			break;
		}
		}

		InstructionPanel->SetContent(Title, Body, HowToLine() + TEXT("\n") + SimNote);
		InstructionPanel->SetProgress(Progress, Progress > 0 ? 5 : 0);
		InstructionPanel->SetFeedback(FeedbackText, FeedbackColor);
		const bool bWarn = FeedbackColor.Equals(TrainerStyle::Warn) && !FeedbackText.IsEmpty();
		const bool bGood = (FeedbackColor.Equals(TrainerStyle::Good) && !FeedbackText.IsEmpty()) || Step == ETrainerStep::Complete;
		InstructionPanel->SetAccentColor(bWarn ? FLinearColor(1.f, 0.4f, 0.f) : bGood ? FLinearColor(0.1f, 0.8f, 0.2f) : TrainerStyle::Accent);
		InstructionPanel->SetDimmed(false);
	}

	if (WorkOrderPanel)
	{
		const bool bAllFound = Identified.Num() >= Parts.Num() && Parts.Num() > 0;
		FString Body = TEXT("LINE 2 / BAY 4\nReported: grinding noise near the rollers.\nTask: inspect, find the fault, report it.\n\nCHECKLIST\n");
		Body += CheckBox(bStopped) + TEXT("Machine stopped (sim)\n");
		Body += CheckBox(bIsolated) + TEXT("Power isolated (sim)\n");
		Body += CheckBox(bAllFound) + FString::Printf(TEXT("Parts identified %d/%d\n"), Identified.Num(), Parts.Num());
		Body += CheckBox(bFaultFound) + TEXT("Fault found\n");
		Body += CheckBox(bDiagnosed) + TEXT("Faulty part named\n");
		Body += CheckBox(bResponded) + TEXT("Reported to maintenance");

		const FString Title = Step == ETrainerStep::Complete ? TEXT("WO #CT-0142 - DONE") : TEXT("Work order #CT-0142");
		WorkOrderPanel->SetContent(Title, Body, TEXT("Training checklist only. Instructor to review."));
		WorkOrderPanel->SetAccentColor(Step == ETrainerStep::Complete ? FLinearColor(0.1f, 0.8f, 0.2f) : TrainerStyle::Accent);
		WorkOrderPanel->SetDimmed(Step == ETrainerStep::Welcome);
	}

	if (bPlaytestMode) { RefreshPlaytestPanel(); }
}

// ---------------------------------------------------------------- playtest

FString ATrainerStepManager::StepName(ETrainerStep S)
{
	switch (S)
	{
	case ETrainerStep::Welcome:          return TEXT("Welcome");
	case ETrainerStep::WorkOrder:        return TEXT("Work order");
	case ETrainerStep::Checklist:        return TEXT("Checklist");
	case ETrainerStep::IdentifyParts:    return TEXT("Identify");
	case ETrainerStep::Inspect:          return TEXT("Inspect");
	case ETrainerStep::DiagnosePart:     return TEXT("Diagnose");
	case ETrainerStep::DiagnoseResponse: return TEXT("Respond");
	case ETrainerStep::Complete:         return TEXT("Complete");
	}
	return TEXT("?");
}

FString ATrainerStepManager::InputName() const
{
	switch (Interactor ? Interactor->GetInputMode() : ETrainerInputMode::Desktop)
	{
	case ETrainerInputMode::Hands:       return TEXT("Hands");
	case ETrainerInputMode::Controllers: return TEXT("Controllers");
	default:                             return TEXT("Desktop");
	}
}

void ATrainerStepManager::PlaytestTick()
{
	// Keyboard hotkeys for whoever runs the playtest at the PC.
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		const bool bR = PC->IsInputKeyDown(EKeys::R);
		const bool bN = PC->IsInputKeyDown(EKeys::N);
		if (bR && !bRWasDown) { RestartTraining(); }
		if (bN && !bNWasDown) { SkipStep(); }
		bRWasDown = bR;
		bNWasDown = bN;
	}

	if (++PlaytestTicks % 10 == 0) { RefreshPlaytestPanel(); }
}

void ATrainerStepManager::RefreshPlaytestPanel()
{
	if (!PlaytestPanel) return;

	const double Now = GetWorld()->GetTimeSeconds();
	const bool bStarted = StartTime > 0.0;
	const int32 Total = !bStarted ? 0 : FMath::FloorToInt((Step == ETrainerStep::Complete ? EndTime : Now) - StartTime);
	const int32 InStep = FMath::FloorToInt(Now - StepEnterTime);
	const int32 Target = FMath::RoundToInt(TargetSeconds);

	FString Body = FString::Printf(TEXT("Run #%d   Time %d:%02d / target %d:%02d\nStep: %s (%ds)\nInput: %s   Retries: %d\nBelt: %s"),
		RunCount + (Step == ETrainerStep::Complete ? 0 : 1), Total / 60, Total % 60, Target / 60, Target % 60, *StepName(Step), InStep, *InputName(), Retries,
		LineFlow ? (LineFlow->IsMoving() ? TEXT("moving") : TEXT("stopped")) : TEXT("-"));
	if (!LastRunSummary.IsEmpty())
	{
		Body += TEXT("\n\nLast run: ") + LastRunSummary;
	}

	PlaytestPanel->SetContent(TEXT("Playtest build"), Body,
		TEXT("PC keys: R restart, N skip step. Red button: reset run.\nLog: Saved/Playtest/playtest_runs.csv"));
	PlaytestPanel->SetAccentColor(Total > Target ? FLinearColor(1.f, 0.4f, 0.f) : TrainerStyle::Accent);
}

void ATrainerStepManager::LogRun()
{
	++RunCount;
	const double Total = EndTime - StartTime;
	const bool bPass = Total <= TargetSeconds;

	static const ETrainerStep Order[] = {
		ETrainerStep::WorkOrder, ETrainerStep::Checklist, ETrainerStep::IdentifyParts, ETrainerStep::Inspect,
		ETrainerStep::DiagnosePart, ETrainerStep::DiagnoseResponse };

	const FString Path = FPaths::ProjectSavedDir() / TEXT("Playtest/playtest_runs.csv");
	FString Line;
	if (!IFileManager::Get().FileExists(*Path))
	{
		Line += TEXT("date,map,input,total_s,workorder_s,checklist_s,identify_s,inspect_s,diagnose_s,respond_s,retries,under_target\n");
	}
	Line += FString::Printf(TEXT("%s,%s,%s,%.1f"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")), *GetWorld()->GetMapName(), *InputName(), Total);
	for (ETrainerStep S : Order)
	{
		const double* Secs = StepSeconds.Find(S);
		Line += FString::Printf(TEXT(",%.1f"), Secs ? *Secs : 0.0);
	}
	Line += FString::Printf(TEXT(",%d,%s\n"), Retries, bPass ? TEXT("yes") : TEXT("no"));
	FFileHelper::SaveStringToFile(Line, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);

	const int32 Secs = FMath::RoundToInt(Total);
	LastRunSummary = FString::Printf(TEXT("%d:%02d, %d retries, %s"), Secs / 60, Secs % 60, Retries, bPass ? TEXT("PASS (under 4 min)") : TEXT("OVER 4 min"));
	UE_LOG(LogTemp, Log, TEXT("Trainer playtest run logged: %s"), *LastRunSummary);
}
