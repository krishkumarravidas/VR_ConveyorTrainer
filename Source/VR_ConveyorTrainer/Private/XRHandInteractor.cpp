#include "XRHandInteractor.h"
#include "TrainerInteractable.h"
#include "TrainerStyle.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HeadMountedDisplayTypes.h"
#include "IXRTrackingSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

static constexpr int32 NumJoints = EHandKeypointCount;
static const EControllerHand Hands[2] = { EControllerHand::Left, EControllerHand::Right };

AXRHandInteractor::AXRHandInteractor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after the pawn has updated tracking

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakeAbsoluteMesh = [&](const FName Name) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetStaticMesh(SphereMesh.Object);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetUsingAbsoluteLocation(true);
		C->SetUsingAbsoluteRotation(true);
		C->SetUsingAbsoluteScale(true);
		C->SetVisibility(false);
		return C;
	};

	for (int32 i = 0; i < 2; ++i)
	{
		const TCHAR* Side = i == 0 ? TEXT("Left") : TEXT("Right");

		UInstancedStaticMeshComponent* Joints = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("%sHandJoints"), Side));
		Joints->SetupAttachment(Root);
		Joints->SetStaticMesh(SphereMesh.Object);
		Joints->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Joints->SetCastShadow(false);
		Joints->SetUsingAbsoluteLocation(true);
		Joints->SetUsingAbsoluteRotation(true);
		Joints->SetUsingAbsoluteScale(true);
		HandJoints.Add(Joints);

		Tips.Add(MakeAbsoluteMesh(*FString::Printf(TEXT("%sTip"), Side)));
	}
	AimDot = MakeAbsoluteMesh(TEXT("AimDot"));
}

void AXRHandInteractor::BeginPlay()
{
	Super::BeginPlay();

	HandMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Hand);
	TipMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Cursor);
	AimMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Cursor);

	AimDot->SetMaterial(0, AimMID);
	for (int32 i = 0; i < 2; ++i)
	{
		HandJoints[i]->SetMaterial(0, HandMID);
		Tips[i]->SetMaterial(0, TipMID);
		HandJoints[i]->ClearInstances();
		for (int32 j = 0; j < NumJoints; ++j)
		{
			HandJoints[i]->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.01f)), true);
		}
		HandJoints[i]->SetVisibility(false);
	}
}

bool AXRHandInteractor::IsInteractable(AActor* Actor)
{
	return Actor && Actor->Implements<UTrainerInteractable>() && ITrainerInteractable::Execute_CanInteract(Actor);
}

void AXRHandInteractor::SetInputMode(ETrainerInputMode NewMode)
{
	if (NewMode == InputMode) return;
	InputMode = NewMode;
	OnInputModeChanged.Broadcast(InputMode);
}

void AXRHandInteractor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager) return;

	IXRTrackingSystem* XR = GEngine && GEngine->XRSystem.IsValid() ? GEngine->XRSystem.Get() : nullptr;
	const bool bHeadset = XR && XR->IsHeadTrackingAllowed();

	for (int32 i = 0; i < 2; ++i)
	{
		Tips_[i].bActive = false;
		Tips_[i].HoverTarget.Reset();
	}
	DesktopHover.Reset();

	if (bHeadset)
	{
		AimDot->SetVisibility(false);
		bool bAnyHand = false, bAnyController = false;
		for (int32 i = 0; i < 2; ++i)
		{
			if (UpdateHand(i))
			{
				bAnyHand = true;
				Tips[i]->SetVisibility(false);
			}
			else
			{
				HandJoints[i]->SetVisibility(false);
				const bool bCtrl = UpdateController(i);
				bAnyController |= bCtrl;
				if (!bCtrl) { HideHand(i); }
			}
		}
		if (bAnyHand) { SetInputMode(ETrainerInputMode::Hands); }
		else if (bAnyController) { SetInputMode(ETrainerInputMode::Controllers); }
	}
	else
	{
		HideHand(0);
		HideHand(1);
		UpdateDesktop(PC, DeltaSeconds);
		SetInputMode(ETrainerInputMode::Desktop);
	}

	// Hover feedback: whatever a fingertip / tip / aim dot is about to touch lights up.
	TSet<TWeakObjectPtr<AActor>> HoveredNow;
	for (int32 i = 0; i < 2; ++i)
	{
		if (Tips_[i].HoverTarget.IsValid()) { HoveredNow.Add(Tips_[i].HoverTarget); }
	}
	if (DesktopHover.IsValid()) { HoveredNow.Add(DesktopHover); }

	for (const TWeakObjectPtr<AActor>& A : HoveredLastFrame)
	{
		if (A.IsValid() && !HoveredNow.Contains(A) && A->Implements<UTrainerInteractable>())
		{
			ITrainerInteractable::Execute_SetHovered(A.Get(), false);
		}
	}
	for (const TWeakObjectPtr<AActor>& A : HoveredNow)
	{
		if (A.IsValid() && !HoveredLastFrame.Contains(A))
		{
			ITrainerInteractable::Execute_SetHovered(A.Get(), true);
		}
	}
	HoveredLastFrame = MoveTemp(HoveredNow);
}

bool AXRHandInteractor::UpdateHand(int32 Index)
{
	FXRHandTrackingState State;
	GEngine->XRSystem->GetHandTrackingState(this, EXRSpaceType::UnrealWorldSpace, Hands[Index], State);
	if (!State.bValid || State.TrackingStatus == ETrackingStatus::NotTracked || State.HandKeyLocations.Num() < NumJoints)
	{
		return false;
	}

	// Joint spheres.
	UInstancedStaticMeshComponent* Joints = HandJoints[Index];
	Joints->SetVisibility(true);
	for (int32 j = 0; j < NumJoints; ++j)
	{
		const float Radius = State.HandKeyRadii.IsValidIndex(j) && State.HandKeyRadii[j] > 0.f ? State.HandKeyRadii[j] : 0.8f;
		const FTransform T(FQuat::Identity, State.HandKeyLocations[j], FVector(FMath::Clamp(Radius, 0.5f, 1.5f) * 2.f / 100.f));
		Joints->UpdateInstanceTransform(j, T, true, j == NumJoints - 1, true);
	}

	Tips_[Index].bActive = true;
	UpdatePoke(Index, State.HandKeyLocations[(int32)EHandKeypoint::IndexTip]);
	return true;
}

bool AXRHandInteractor::UpdateController(int32 Index)
{
	FXRMotionControllerState State;
	GEngine->XRSystem->GetMotionControllerState(this, EXRSpaceType::UnrealWorldSpace, Hands[Index], EXRControllerPoseType::Aim, State);
	if (!State.bValid || State.TrackingStatus == ETrackingStatus::NotTracked)
	{
		Tips[Index]->SetVisibility(false);
		return false;
	}

	const FVector Tip = State.ControllerLocation + State.ControllerRotation.GetForwardVector() * ControllerTipOffset;
	Tips[Index]->SetVisibility(true);
	Tips[Index]->SetWorldLocation(Tip);
	Tips[Index]->SetWorldScale3D(FVector(PokeRadius * 2.f / 100.f));

	Tips_[Index].bActive = true;
	UpdatePoke(Index, Tip);
	return true;
}

void AXRHandInteractor::UpdatePoke(int32 Index, const FVector& Tip)
{
	FTipState& P = Tips_[Index];
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TrainerPoke), false, this);
	if (APawn* Pawn = GetWorld()->GetFirstPlayerController()->GetPawn()) { Params.AddIgnoredActor(Pawn); }

	// Hover: the nearest interactable within HoverRadius.
	TArray<FOverlapResult> Near;
	GetWorld()->OverlapMultiByChannel(Near, Tip, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(HoverRadius), Params);
	float Best = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Near)
	{
		AActor* A = O.GetActor();
		if (!IsInteractable(A) || !O.Component.IsValid()) continue;
		FVector Closest;
		const float D = O.Component->GetDistanceToCollision(Tip, Closest);
		if (D >= 0.f && D < Best) { Best = D; P.HoverTarget = A; }
	}

	// Touch: overlap at the tip, plus a sweep from last frame's tip so fast pokes do not pass through thin buttons.
	TSet<TWeakObjectPtr<AActor>> Now;
	TArray<FOverlapResult> Touching;
	GetWorld()->OverlapMultiByChannel(Touching, Tip, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(PokeRadius), Params);
	for (const FOverlapResult& O : Touching)
	{
		if (IsInteractable(O.GetActor())) { Now.Add(O.GetActor()); }
	}
	if (P.bHasLastTip && FVector::DistSquared(P.LastTip, Tip) > 0.25f)
	{
		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, P.LastTip, Tip, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(PokeRadius), Params)
			&& IsInteractable(Hit.GetActor()))
		{
			Now.Add(Hit.GetActor());
		}
	}
	P.LastTip = Tip;
	P.bHasLastTip = true;

	for (const TWeakObjectPtr<AActor>& A : Now)
	{
		if (A.IsValid() && !P.Poking.Contains(A))
		{
			ITrainerInteractable::Execute_Interact(A.Get()); // fires once per touch; pull the finger out to press again
		}
	}
	P.Poking = MoveTemp(Now);
}

void AXRHandInteractor::UpdateDesktop(APlayerController* PC, float DeltaSeconds)
{
	APawn* Pawn = PC->GetPawn();
	FixDesktopCamera(Pawn);
	UCameraComponent* Cam = Pawn ? Pawn->FindComponentByClass<UCameraComponent>() : nullptr;

	// Hold right mouse to look around.
	if (Cam && PC->IsInputKeyDown(EKeys::RightMouseButton))
	{
		float DX = 0.f, DY = 0.f;
		PC->GetInputMouseDelta(DX, DY);
		FRotator R = Cam->GetRelativeRotation();
		R.Yaw += DX * 1.5f;
		R.Pitch = FMath::Clamp(R.Pitch + DY * 1.5f, -80.f, 80.f);
		R.Roll = 0.f;
		Cam->SetRelativeRotation(R);
	}

	// WASD walks on the floor plane (so desktop testers step up to things like a VR user would).
	if (Pawn && Cam)
	{
		FVector Move = FVector::ZeroVector;
		const FRotator Yaw(0.f, Cam->GetComponentRotation().Yaw, 0.f);
		const FVector Fwd = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
		if (PC->IsInputKeyDown(EKeys::W)) Move += Fwd;
		if (PC->IsInputKeyDown(EKeys::S)) Move -= Fwd;
		if (PC->IsInputKeyDown(EKeys::D)) Move += Right;
		if (PC->IsInputKeyDown(EKeys::A)) Move -= Right;
		if (!Move.IsNearlyZero())
		{
			Pawn->AddActorWorldOffset(Move.GetSafeNormal() * DesktopWalkSpeed * DeltaSeconds, false);
		}
	}

	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FVector Dir = PC->PlayerCameraManager->GetCameraRotation().Vector();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TrainerDesktopAim), true, this);
	if (Pawn) { Params.AddIgnoredActor(Pawn); }
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, CamLoc + Dir * 1000.f, ECC_Visibility, Params);
	const bool bInReach = bHit && Hit.Distance <= DesktopReach;

	if (bInReach && IsInteractable(Hit.GetActor()))
	{
		DesktopHover = Hit.GetActor();
	}

	// Aim dot: yellow and bigger on something touchable in reach, small grey otherwise.
	AimDot->SetVisibility(bHit);
	if (bHit)
	{
		const bool bOnTarget = DesktopHover.IsValid();
		AimDot->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * 0.3f);
		AimDot->SetWorldScale3D(FVector((bOnTarget ? 1.8f : 0.9f) / 100.f));
		if (AimMID) { AimMID->SetVectorParameterValue(TEXT("Color"), bOnTarget ? TrainerStyle::Cursor : (bInReach ? FLinearColor(0.8f, 0.8f, 0.8f) : FLinearColor(0.3f, 0.3f, 0.3f))); }
	}

	// Left-click = touch.
	const bool bDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	if (bDown && !bMouseWasDown && DesktopHover.IsValid())
	{
		ITrainerInteractable::Execute_Interact(DesktopHover.Get());
	}
	bMouseWasDown = bDown;
}

void AXRHandInteractor::FixDesktopCamera(APawn* Pawn)
{
	if (!Pawn) return;
	UCameraComponent* Cam = Pawn->FindComponentByClass<UCameraComponent>();
	if (!Cam) return;

	if (!bDesktopCameraFixed)
	{
		// Find the floor under the pawn once.
		const FVector Start = Pawn->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TrainerFloor), false, Pawn);
		Params.AddIgnoredActor(this);
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
		{
			return;
		}
		DesktopFloorZ = Hit.ImpactPoint.Z;
		bDesktopCameraFixed = true;
		UE_LOG(LogTemp, Log, TEXT("XRHandInteractor: desktop mode, floor at %.1f (hit %s), eye at %.1f"), DesktopFloorZ, *GetNameSafe(Hit.GetActor()), DesktopFloorZ + DesktopEyeHeight);
	}

	// Keep the desktop eye at standing height above the floor.
	const FVector L = Cam->GetComponentLocation();
	const float WantZ = DesktopFloorZ + DesktopEyeHeight;
	if (!FMath::IsNearlyEqual(L.Z, WantZ, 0.5f))
	{
		Cam->SetWorldLocation(FVector(L.X, L.Y, WantZ));
	}
}

void AXRHandInteractor::HideHand(int32 Index)
{
	HandJoints[Index]->SetVisibility(false);
	Tips[Index]->SetVisibility(false);
	Tips_[Index].Poking.Reset();
	Tips_[Index].bHasLastTip = false;
}
