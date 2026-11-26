#include "TrainerPanel.h"
#include "TrainerStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

static int32 CountLines(const FString& S)
{
	if (S.IsEmpty()) return 0;
	int32 N = 1;
	for (TCHAR C : S) { if (C == TEXT('\n')) ++N; }
	return N;
}

ATrainerPanel::ATrainerPanel()
{
	PrimaryActorTick.bCanEverTick = false;
	AccentColor = TrainerStyle::Accent;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakeMesh = [&](const FName Name) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetStaticMesh(CubeMesh.Object);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		return C;
	};
	auto MakeText = [&](const FName Name) {
		UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(Name);
		T->SetupAttachment(Root);
		T->SetHorizontalAlignment(EHTA_Left);
		T->SetVerticalAlignment(EVRTA_TextTop);
		T->SetTextRenderColor(TrainerStyle::Ink.ToFColor(true));
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		T->SetCastShadow(false);
		return T;
	};

	Backing = MakeMesh(TEXT("Backing"));
	Backing->SetCastShadow(true);
	Sheet = MakeMesh(TEXT("Sheet"));
	StatusStrip = MakeMesh(TEXT("StatusStrip"));
	TitleRule = MakeMesh(TEXT("TitleRule"));
	TapeA = MakeMesh(TEXT("TapeA"));
	TapeB = MakeMesh(TEXT("TapeB"));
	Clip = MakeMesh(TEXT("Clip"));

	TitleText = MakeText(TEXT("TitleText"));
	StepText = MakeText(TEXT("StepText"));
	StepText->SetHorizontalAlignment(EHTA_Right);
	BodyText = MakeText(TEXT("BodyText"));
	FeedbackText = MakeText(TEXT("FeedbackText"));
	FooterText = MakeText(TEXT("FooterText"));
	FeedbackText->SetText(FText::GetEmpty());
}

void ATrainerPanel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitVisuals();
}

void ATrainerPanel::BeginPlay()
{
	Super::BeginPlay();
	InitVisuals();
}

void ATrainerPanel::InitVisuals()
{
	// Transient MIDs are not saved with the level, so build them in both OnConstruction and BeginPlay.
	if (!BackingMID) { BackingMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Frame, true); }
	if (!SheetMID) { SheetMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Paper, true); }
	if (!StripMID) { StripMID = TrainerStyle::MakeColorMID(this, AccentColor); }
	if (!InkMID) { InkMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Ink); }
	if (!TapeMID) { TapeMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Tape, true); }
	if (!ClipMID) { ClipMID = TrainerStyle::MakeColorMID(this, TrainerStyle::Steel, true); }

	FLinearColor BackingColor = TrainerStyle::Frame;
	FLinearColor SheetColor = TrainerStyle::Paper;
	if (Style == ETrainerPanelStyle::Clipboard) { BackingColor = TrainerStyle::Clipboard; }
	if (Style == ETrainerPanelStyle::Sign) { BackingColor = TrainerStyle::Ink; SheetColor = TrainerStyle::Accent * 0.8f; }
	if (BackingMID) BackingMID->SetVectorParameterValue(TEXT("Color"), BackingColor);
	if (SheetMID) SheetMID->SetVectorParameterValue(TEXT("Color"), SheetColor);

	Backing->SetMaterial(0, BackingMID);
	Sheet->SetMaterial(0, SheetMID);
	StatusStrip->SetMaterial(0, StripMID);
	TitleRule->SetMaterial(0, InkMID);
	TapeA->SetMaterial(0, TapeMID);
	TapeB->SetMaterial(0, TapeMID);
	Clip->SetMaterial(0, ClipMID);

	Layout();
	ApplyColors();
}

int32 ATrainerPanel::CharsPerLine(float Size, bool bCaps) const
{
	// The default font averages roughly 0.55 x world size per character (wider for capitals).
	const float Border = Style == ETrainerPanelStyle::Clipboard ? 4.f : 2.f;
	const float Usable = Width - 2.f * Border - 8.f - (Style == ETrainerPanelStyle::Sign ? 0.f : 2.f);
	return FMath::Max(8, FMath::FloorToInt(Usable / (Size * (bCaps ? 0.68f : 0.55f))));
}

void ATrainerPanel::Layout()
{
	const float HalfW = Width * 0.5f;
	const float HalfH = Height * 0.5f;
	const float Pad = 4.f;
	const float Front = 0.5f; // text sits just in front of the sheet

	const bool bClip = Style == ETrainerPanelStyle::Clipboard;
	const bool bBoard = Style == ETrainerPanelStyle::Whiteboard;
	const bool bStrip = Style != ETrainerPanelStyle::Sign;
	const float Border = bClip ? 4.f : (bBoard ? 2.f : 1.5f);
	const float BorderTop = bClip ? 8.f : Border;

	// Cube is 100cm: X = depth, Y = width, Z = height. Backing front face sits at X = 0.
	Backing->SetRelativeLocation(FVector(-1.f, 0.f, 0.f));
	Backing->SetRelativeScale3D(FVector(0.02f, Width / 100.f, Height / 100.f));

	const float SheetW = Width - 2.f * Border;
	const float SheetH = Height - Border - BorderTop;
	const float SheetMidZ = (-HalfH + Border + HalfH - BorderTop) * 0.5f;
	Sheet->SetRelativeLocation(FVector(0.15f, 0.f, SheetMidZ));
	Sheet->SetRelativeScale3D(FVector(0.003f, SheetW / 100.f, SheetH / 100.f));

	const float SheetLeft = SheetW * 0.5f;          // +Y edge (viewer's left)
	const float SheetTop = HalfH - BorderTop;
	const float SheetBottom = -HalfH + Border;

	// Coloured status strip down the left edge of the sheet.
	StatusStrip->SetVisibility(bStrip);
	StatusStrip->SetRelativeLocation(FVector(0.35f, SheetLeft - 0.9f, SheetMidZ));
	StatusStrip->SetRelativeScale3D(FVector(0.002f, 0.018f, SheetH / 100.f));

	// Masking tape on the top corners (whiteboard only) and a clip (clipboard only).
	TapeA->SetVisibility(bBoard);
	TapeB->SetVisibility(bBoard);
	TapeA->SetRelativeLocation(FVector(0.6f, HalfW - 3.f, HalfH - 2.f));
	TapeA->SetRelativeRotation(FRotator(0.f, 0.f, -35.f));
	TapeA->SetRelativeScale3D(FVector(0.002f, 0.12f, 0.035f));
	TapeB->SetRelativeLocation(FVector(0.6f, -HalfW + 3.f, HalfH - 2.f));
	TapeB->SetRelativeRotation(FRotator(0.f, 0.f, 35.f));
	TapeB->SetRelativeScale3D(FVector(0.002f, 0.12f, 0.035f));
	Clip->SetVisibility(bClip);
	Clip->SetRelativeLocation(FVector(1.0f, 0.f, HalfH - 4.f));
	Clip->SetRelativeScale3D(FVector(0.025f, FMath::Min(22.f, Width * 0.3f) / 100.f, 0.06f));

	const float Left = SheetLeft - Pad - (bStrip ? 1.8f : 0.f);
	const float Right = -SheetLeft + Pad;
	float Z = SheetTop - Pad * 0.75f;

	// "STEP n/N" tag in the top-right corner.
	const float StepSize = TitleSize * 0.62f;
	StepText->SetWorldSize(StepSize);
	StepText->SetText(FText::FromString(ProgressTotal > 0 ? FString::Printf(TEXT("STEP %d/%d"), ProgressCurrent, ProgressTotal) : FString()));
	StepText->SetRelativeLocation(FVector(Front, Right, Z - (TitleSize - StepSize) * 0.4f));
	const int32 StepChars = ProgressTotal > 0 ? FMath::CeilToInt(9 * StepSize / TitleSize) + 1 : 0;

	const FString WrappedTitle = TrainerStyle::Wrap(Title.ToUpper(), CharsPerLine(TitleSize, true) - StepChars);
	TitleText->SetWorldSize(TitleSize);
	TitleText->SetText(FText::FromString(WrappedTitle));
	TitleText->SetRelativeLocation(FVector(Front, Left, Z));
	Z -= CountLines(WrappedTitle) * TitleSize * 1.1f + 1.2f;

	TitleRule->SetVisibility(!WrappedTitle.IsEmpty());
	TitleRule->SetRelativeLocation(FVector(0.4f, (Left + Right) * 0.5f, Z));
	TitleRule->SetRelativeScale3D(FVector(0.002f, (Left - Right) / 100.f, 0.0035f));
	Z -= 2.5f;

	const FString WrappedBody = TrainerStyle::Wrap(Body, CharsPerLine(BodySize));
	BodyText->SetWorldSize(BodySize);
	BodyText->SetText(FText::FromString(WrappedBody));
	BodyText->SetRelativeLocation(FVector(Front, Left, Z));
	Z -= CountLines(WrappedBody) * BodySize * 1.15f + 1.5f;

	FeedbackText->SetWorldSize(BodySize);
	FeedbackText->SetRelativeLocation(FVector(Front, Left, Z));

	const FString WrappedFooter = TrainerStyle::Wrap(Footer, CharsPerLine(FooterSize));
	FooterText->SetWorldSize(FooterSize);
	FooterText->SetText(FText::FromString(WrappedFooter));
	FooterText->SetRelativeLocation(FVector(Front, Left, SheetBottom + Pad * 0.6f + CountLines(WrappedFooter) * FooterSize * 1.15f));
}

void ATrainerPanel::ApplyColors()
{
	const FLinearColor Base = TrainerStyle::Ink;
	const FLinearColor Faded = FLinearColor::LerpUsingHSV(Base, TrainerStyle::Paper, 0.55f);
	const FLinearColor Main = bDimmed ? Faded : Base;
	TitleText->SetTextRenderColor(Main.ToFColor(true));
	BodyText->SetTextRenderColor(Main.ToFColor(true));
	StepText->SetTextRenderColor((bDimmed ? Faded : TrainerStyle::DimInk).ToFColor(true));
	FooterText->SetTextRenderColor((bDimmed ? Faded : TrainerStyle::DimInk).ToFColor(true));
	if (StripMID) { StripMID->SetVectorParameterValue(TEXT("Color"), bDimmed ? AccentColor * 0.4f : AccentColor); }
}

void ATrainerPanel::SetContent(const FString& InTitle, const FString& InBody, const FString& InFooter)
{
	Title = InTitle;
	Body = InBody;
	Footer = InFooter;
	Layout();
	ApplyColors();
}

void ATrainerPanel::SetFeedback(const FString& InText, FLinearColor Color)
{
	FeedbackText->SetText(FText::FromString(TrainerStyle::Wrap(InText, CharsPerLine(BodySize))));
	FeedbackText->SetTextRenderColor(Color.ToFColor(true));
	FeedbackText->SetVisibility(!InText.IsEmpty());
}

void ATrainerPanel::SetAccentColor(FLinearColor Color)
{
	AccentColor = Color;
	ApplyColors();
}

void ATrainerPanel::SetProgress(int32 Current, int32 Total)
{
	ProgressCurrent = Current;
	ProgressTotal = FMath::Max(Total, 0);
	Layout();
	ApplyColors();
}

void ATrainerPanel::SetDimmed(bool bInDimmed)
{
	bDimmed = bInDimmed;
	ApplyColors();
}
