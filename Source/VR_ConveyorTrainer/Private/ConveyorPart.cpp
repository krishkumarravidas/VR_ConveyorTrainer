#include "ConveyorPart.h"
#include "TrainerStyle.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

static constexpr float LabelTextSize = 3.2f;

AConveyorPart::AConveyorPart()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TouchZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TouchZone"));
	TouchZone->SetupAttachment(Mesh);
	TouchZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TouchZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	TouchZone->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TouchZone->SetGenerateOverlapEvents(false);

	LabelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LabelRoot"));
	LabelRoot->SetupAttachment(Mesh);
	LabelRoot->SetUsingAbsoluteScale(true);

	LabelBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LabelBackground"));
	LabelBackground->SetupAttachment(LabelRoot);
	LabelBackground->SetStaticMesh(CubeMesh.Object);
	LabelBackground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LabelBackground->SetCastShadow(false);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(LabelRoot);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetWorldSize(LabelTextSize);
	LabelText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LabelText->SetCastShadow(false);
	LabelText->SetRelativeLocation(FVector(0.4f, 0.f, 0.f));

	LeaderLine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeaderLine"));
	LeaderLine->SetupAttachment(Mesh);
	LeaderLine->SetStaticMesh(CubeMesh.Object);
	LeaderLine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeaderLine->SetCastShadow(false);
	LeaderLine->SetUsingAbsoluteLocation(true);
	LeaderLine->SetUsingAbsoluteRotation(true);
	LeaderLine->SetUsingAbsoluteScale(true);
}

void AConveyorPart::InitVisuals()
{
	if (PartMesh)
	{
		Mesh->SetStaticMesh(PartMesh);
	}
	if (!LabelBgMID) { LabelBgMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Accent); }
	if (!LeaderMID) { LeaderMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Ink); }
	LabelBackground->SetMaterial(0, LabelBgMID);
	LeaderLine->SetMaterial(0, LeaderMID);
	LabelRoot->SetRelativeLocation(LabelOffset);
	UpdateTouchZone();
	ApplyMaterials();
	RefreshLabel();
}

void AConveyorPart::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitVisuals();
}

void AConveyorPart::BeginPlay()
{
	Super::BeginPlay();
	BaseLocation = GetActorLocation();
	BaseRotation = GetActorRotation();
	bHasBaseTransform = true;
	InitVisuals();
	ApplyFaultState(true);
}

void AConveyorPart::SetFaulty(bool bInFaulty)
{
	bIsFaulty = bInFaulty;
	ApplyFaultState(true);
	ApplyMaterials();
	RefreshLabel();
}

void AConveyorPart::ApplyFaultState(bool bMove)
{
	// Extra fault visuals only exist while faulty (game only; the editor keeps them visible for layout).
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		for (AActor* A : FaultVisuals)
		{
			if (A) { A->SetActorHiddenInGame(!bIsFaulty); }
		}
	}
	if (bMove && bMoveWhenFaulty && bHasBaseTransform)
	{
		SetActorLocationAndRotation(bIsFaulty ? BaseLocation + FaultLocationOffset : BaseLocation,
			bIsFaulty ? FaultRotation : BaseRotation);
	}
}

void AConveyorPart::UpdateTouchZone()
{
	UStaticMesh* SM = Mesh->GetStaticMesh();
	if (!SM) return;

	// Box in mesh space, padded by TouchMargin world-cm (divide by scale so the margin is not stretched).
	const FBox B = SM->GetBoundingBox();
	const FVector S = GetActorScale3D().GetAbs().ComponentMax(FVector(0.01f));
	const FVector Extent = B.GetExtent() + FVector(TouchMargin) / S;
	TouchZone->SetRelativeLocation(B.GetCenter());
	TouchZone->SetBoxExtent(Extent, false);
}

void AConveyorPart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the tag turned toward the viewer (yaw only, so text stays upright).
	if (LabelMode == EPartLabelMode::Hidden) return;
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FVector ToCam = Cam->GetCameraLocation() - LabelRoot->GetComponentLocation();
		ToCam.Z = 0.f;
		if (!ToCam.IsNearlyZero())
		{
			LabelRoot->SetWorldRotation(ToCam.Rotation());
		}
	}
	UpdateLeader();
}

void AConveyorPart::UpdateLeader()
{
	const FVector From = LabelRoot->GetComponentLocation() - FVector(0.f, 0.f, LabelHeight * 0.5f);
	const FVector To = TouchZone->Bounds.Origin;
	const FVector D = To - From;
	const float Len = D.Size();
	if (Len < 1.f)
	{
		LeaderLine->SetVisibility(false);
		return;
	}
	LeaderLine->SetWorldLocation(From + D * 0.5f);
	LeaderLine->SetWorldRotation(FRotationMatrix::MakeFromX(D).Rotator());
	LeaderLine->SetWorldScale3D(FVector(Len / 100.f, 0.003f, 0.003f));
}

void AConveyorPart::ApplyMaterials()
{
	UStaticMesh* SM = Mesh->GetStaticMesh();
	if (!SM) return;

	const int32 NumSlots = SM->GetStaticMaterials().Num();
	if (IsHighlighted())
	{
		UMaterialInterface* HL = HighlightMaterial;
		if (!HL)
		{
			if (!FallbackHighlightMID) { FallbackHighlightMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Accent); }
			HL = FallbackHighlightMID;
		}
		for (int32 i = 0; i < NumSlots; ++i) { Mesh->SetMaterial(i, HL); }
		return;
	}

	for (int32 i = 0; i < NumSlots; ++i)
	{
		UMaterialInterface* M = SM->GetMaterial(i);
		if (MaterialOverrides.IsValidIndex(i) && MaterialOverrides[i]) { M = MaterialOverrides[i]; }
		if (bIsFaulty && FaultMaterialOverrides.IsValidIndex(i) && FaultMaterialOverrides[i]) { M = FaultMaterialOverrides[i]; }
		Mesh->SetMaterial(i, M);
	}
}

void AConveyorPart::RefreshLabel()
{
	FString Text;
	FLinearColor TagColor = TrainerStyle::Accent;
	float Size = LabelTextSize;
	switch (LabelMode)
	{
	case EPartLabelMode::Marker:      Text = TEXT("?"); Size = LabelTextSize * 1.8f; break;
	case EPartLabelMode::NameAndRole: Text = PartName.ToUpper() + TEXT("\n") + TrainerStyle::Wrap(Description, 28); break;
	case EPartLabelMode::NameOnly:    Text = PartName.ToUpper(); TagColor = TrainerStyle::Paper; break;
	case EPartLabelMode::InspectNote: Text = PartName.ToUpper() + TEXT("\n") + TrainerStyle::Wrap(GetInspectNote(), 28); TagColor = bIsFaulty ? FLinearColor(1.f, 0.4f, 0.05f) : TrainerStyle::Paper; break;
	default: break;
	}

	const bool bShow = LabelMode != EPartLabelMode::Hidden;
	LabelRoot->SetVisibility(bShow, true);
	LeaderLine->SetVisibility(bShow);
	SetActorTickEnabled(bShow);
	if (!bShow) return;

	LabelText->SetWorldSize(Size);
	LabelText->SetText(FText::FromString(Text));
	LabelText->SetTextRenderColor(TrainerStyle::Ink.ToFColor(true));
	if (LabelBgMID) { LabelBgMID->SetVectorParameterValue(TEXT("Color"), TagColor); }

	int32 Lines = 1, Longest = 0, Cur = 0;
	for (TCHAR C : Text)
	{
		if (C == TEXT('\n')) { ++Lines; Longest = FMath::Max(Longest, Cur); Cur = 0; }
		else { ++Cur; }
	}
	Longest = FMath::Max(Longest, Cur);
	const float W = FMath::Max(Longest * Size * 0.6f + 4.f, Size * 1.6f);
	const float H = Lines * Size * 1.15f + 2.5f;
	LabelHeight = H;
	LabelBackground->SetRelativeScale3D(FVector(0.005f, W / 100.f, H / 100.f));
	LabelBackground->SetRelativeLocation(FVector(-0.3f, 0.f, 0.f));
	UpdateLeader();
}

void AConveyorPart::SetSelectable(bool bInSelectable)
{
	bSelectable = bInSelectable;
	if (!bSelectable && bHoveredState)
	{
		bHoveredState = false;
		ApplyMaterials();
	}
}

void AConveyorPart::SetLabelMode(EPartLabelMode Mode)
{
	LabelMode = Mode;
	RefreshLabel();
}

void AConveyorPart::SetHighlighted(bool bHighlighted)
{
	bForcedHighlight = bHighlighted;
	ApplyMaterials();
}

bool AConveyorPart::CanInteract_Implementation() const
{
	return bSelectable;
}

void AConveyorPart::SetHovered_Implementation(bool bHovered)
{
	bHoveredState = bHovered && bSelectable;
	ApplyMaterials();
}

void AConveyorPart::Interact_Implementation()
{
	if (!bSelectable) return;
	OnPartSelected();
	OnSelected.Broadcast(this);
}
