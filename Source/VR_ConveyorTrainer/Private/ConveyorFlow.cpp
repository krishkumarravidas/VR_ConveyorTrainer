#include "ConveyorFlow.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AConveyorFlow::AConveyorFlow()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakeISM = [&](const FName Name) {
		UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetStaticMesh(CubeMesh.Object);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);
		return C;
	};
	BoxesA = MakeISM(TEXT("BoxesA"));
	BoxesB = MakeISM(TEXT("BoxesB"));
}

void AConveyorFlow::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildBoxes();
}

void AConveyorFlow::BeginPlay()
{
	Super::BeginPlay();
	CurrentSpeed = bRunning ? Speed : 0.f;
	BuildBoxes();
}

void AConveyorFlow::SetRunning(bool bInRunning)
{
	bRunning = bInRunning;
}

void AConveyorFlow::BuildBoxes()
{
	if (BoxMaterial) { BoxesA->SetMaterial(0, BoxMaterial); }
	if (BoxMaterialAlt || BoxMaterial) { BoxesB->SetMaterial(0, BoxMaterialAlt ? BoxMaterialAlt.Get() : BoxMaterial.Get()); }

	BoxesA->ClearInstances();
	BoxesB->ClearInstances();
	Boxes.Reset();

	const float Len = FVector::Dist(PathStart, PathEnd);
	if (Len < 1.f || Spacing < 1.f) return;

	const int32 Num = FMath::CeilToInt(Len / Spacing) + 1;
	FRandomStream Rand(Seed);
	for (int32 i = 0; i < Num; ++i)
	{
		FBoxInfo B;
		B.bAlt = Rand.FRand() < 0.4f;
		B.Size = FVector(Rand.FRandRange(BoxSizeMin.X, BoxSizeMax.X), Rand.FRandRange(BoxSizeMin.Y, BoxSizeMax.Y), Rand.FRandRange(BoxSizeMin.Z, BoxSizeMax.Z));
		B.Yaw = Rand.FRandRange(-10.f, 10.f);
		B.Lateral = Rand.FRandRange(-6.f, 6.f);
		UInstancedStaticMeshComponent* ISM = B.bAlt ? BoxesB : BoxesA;
		B.Instance = ISM->AddInstance(FTransform::Identity, false);
		Boxes.Add(B);
	}
	UpdateBoxes();
}

void AConveyorFlow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, bRunning ? Speed : 0.f, DeltaSeconds, Acceleration);
	if (CurrentSpeed <= 0.f) return;
	Offset += CurrentSpeed * DeltaSeconds;
	UpdateBoxes();
}

void AConveyorFlow::UpdateBoxes()
{
	const float Len = FVector::Dist(PathStart, PathEnd);
	if (Len < 1.f || Boxes.Num() == 0) return;

	const FVector Dir = (PathEnd - PathStart) / Len;
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir).GetSafeNormal();
	const FRotator Facing = Dir.Rotation();
	const float Loop = Boxes.Num() * Spacing;
	Offset = FMath::Fmod(Offset, Loop);

	for (int32 i = 0; i < Boxes.Num(); ++i)
	{
		const FBoxInfo& B = Boxes[i];
		const float D = FMath::Fmod(Offset + i * Spacing, Loop);

		float Scale = 0.f;
		if (D <= Len)
		{
			Scale = FMath::Clamp(FMath::Min(D, Len - D) / FMath::Max(FadeLength, 0.01f), 0.f, 1.f);
		}
		const FVector Size = B.Size * FMath::Max(Scale, 0.001f);
		const FVector Loc = PathStart + Dir * FMath::Min(D, Len) + Side * B.Lateral + FVector(0.f, 0.f, Size.Z * 0.5f);
		const FTransform T(FRotator(0.f, Facing.Yaw + B.Yaw, 0.f), Loc, Size / 100.f);

		UInstancedStaticMeshComponent* ISM = B.bAlt ? BoxesB : BoxesA;
		ISM->UpdateInstanceTransform(B.Instance, T, false, false, true);
	}
	BoxesA->MarkRenderStateDirty();
	BoxesB->MarkRenderStateDirty();
}
