#include "TrainerButton.h"
#include "TrainerStyle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

static constexpr float PressCooldown = 0.4f;
static constexpr float HitDepth = 3.25f;   // hitbox half-depth; front reaches ~1.5cm past the cap
static constexpr float BackX = -HitDepth;  // enclosure back face sits on the stand surface

ATrainerButton::ATrainerButton()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	RootComponent = Hitbox;
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Hitbox->SetGenerateOverlapEvents(false);

	auto MakeMesh = [&](const FName Name) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Hitbox);
		C->SetStaticMesh(CubeMesh.Object);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		return C;
	};
	Plate = MakeMesh(TEXT("Plate"));
	Bezel = MakeMesh(TEXT("Bezel"));
	Cap = MakeMesh(TEXT("Cap"));
	LabelStrip = MakeMesh(TEXT("LabelStrip"));

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(Hitbox);
	LabelText->SetHorizontalAlignment(EHTA_Left);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LabelText->SetCastShadow(false);
}

void ATrainerButton::InitVisuals()
{
	if (!PlateMID) { PlateMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Frame, true); }
	if (!CapMID) { CapMID = TrainerStyle::MakeColorMID(this, CapColor); }
	if (!StripMID) { StripMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Paper, true); }
	Plate->SetMaterial(0, PlateMID);
	Bezel->SetMaterial(0, PlateMID);
	Cap->SetMaterial(0, CapMID);
	LabelStrip->SetMaterial(0, StripMID);
	Layout();
	UpdateColor();
}

void ATrainerButton::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitVisuals();
}

void ATrainerButton::BeginPlay()
{
	Super::BeginPlay();
	InitVisuals();
}

void ATrainerButton::Layout()
{
	const float HalfW = Size.X * 0.5f;
	Hitbox->SetBoxExtent(FVector(HitDepth, HalfW, Size.Y * 0.5f));

	// Enclosure plate: 2cm deep, front face at BackX + 2.
	const float PlateFront = BackX + 2.f;
	Plate->SetRelativeLocation(FVector(BackX + 1.f, 0.f, 0.f));
	Plate->SetRelativeScale3D(FVector(0.02f, Size.X / 100.f, Size.Y / 100.f));

	// Square cap on the viewer's left (+Y), sitting in a bezel.
	const float CapSize = FMath::Max(4.f, Size.Y - 2.f);
	const float CapY = HalfW - 1.f - CapSize * 0.5f;
	Bezel->SetRelativeLocation(FVector(PlateFront + 0.4f, CapY, 0.f));
	Bezel->SetRelativeScale3D(FVector(0.008f, (CapSize + 1.4f) / 100.f, (CapSize + 1.4f) / 100.f));

	const float PressOffset = bPressAnim ? -1.2f : 0.f;
	Cap->SetRelativeLocation(FVector(PlateFront + 0.8f + 1.1f + PressOffset, CapY, 0.f));
	Cap->SetRelativeScale3D(FVector(0.022f, CapSize / 100.f, CapSize / 100.f));

	// Label strip fills the rest of the plate.
	const float StripLeft = CapY - CapSize * 0.5f - 1.6f;
	const float StripRight = -HalfW + 1.f;
	const float StripW = FMath::Max(1.f, StripLeft - StripRight);
	LabelStrip->SetRelativeLocation(FVector(PlateFront + 0.15f, (StripLeft + StripRight) * 0.5f, 0.f));
	LabelStrip->SetRelativeScale3D(FVector(0.003f, StripW / 100.f, (Size.Y - 2.6f) / 100.f));

	LabelText->SetWorldSize(TextSize);
	LabelText->SetText(FText::FromString(Label.ToUpper()));
	LabelText->SetRelativeLocation(FVector(PlateFront + 0.45f, StripLeft - 1.5f, 0.f));
}

void ATrainerButton::UpdateColor()
{
	LabelText->SetTextRenderColor(TrainerStyle::Ink.ToFColor(true));

	FLinearColor C = CapColor;
	if (bToggled) { C = TrainerStyle::CapLit; }
	if (bHoveredState) { C = (C * 1.25f).GetClamped(); }
	if (bPressAnim) { C = FLinearColor(1.f, 1.f, 0.85f); }
	if (CapMID) { CapMID->SetVectorParameterValue(TEXT("Color"), C); }

	if (StripMID)
	{
		const FLinearColor S = bHoveredState ? FLinearColor(0.95f, 0.85f, 0.45f) : (bToggled ? FLinearColor(0.6f, 0.8f, 0.6f) : TrainerStyle::Paper);
		StripMID->SetVectorParameterValue(TEXT("Color"), S);
	}
}

void ATrainerButton::SetLabel(const FString& InLabel)
{
	Label = InLabel;
	Layout();
}

void ATrainerButton::SetButtonEnabled(bool bEnabled)
{
	bEnabledState = bEnabled;
	SetActorHiddenInGame(!bEnabled);
	SetActorEnableCollision(bEnabled);
	if (!bEnabled)
	{
		bHoveredState = false;
		UpdateColor();
	}
}

void ATrainerButton::SetToggled(bool bOn)
{
	bToggled = bOn;
	if (bIsToggle)
	{
		const FString Base = Label.Replace(TEXT("[x] "), TEXT("")).Replace(TEXT("[ ] "), TEXT(""));
		Label = (bOn ? TEXT("[x] ") : TEXT("[ ] ")) + Base;
		Layout();
	}
	UpdateColor();
}

bool ATrainerButton::CanInteract_Implementation() const
{
	return bEnabledState;
}

void ATrainerButton::SetHovered_Implementation(bool bHovered)
{
	bHoveredState = bHovered && bEnabledState;
	UpdateColor();
}

void ATrainerButton::Interact_Implementation()
{
	if (!bEnabledState) return;

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastPressTime < PressCooldown) return;
	LastPressTime = Now;

	if (bIsToggle)
	{
		SetToggled(!bToggled);
	}

	bPressAnim = true;
	Layout();
	UpdateColor();
	GetWorldTimerManager().SetTimer(PressTimer, this, &ATrainerButton::EndPressAnim, 0.15f, false);

	OnPressed.Broadcast(this);
}

void ATrainerButton::EndPressAnim()
{
	bPressAnim = false;
	Layout();
	UpdateColor();
}
