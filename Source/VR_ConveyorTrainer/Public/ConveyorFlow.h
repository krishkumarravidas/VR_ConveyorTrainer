#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConveyorFlow.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

/**
 * Cardboard boxes riding along a straight belt path. Purely visual (no physics, no collision).
 * Boxes grow in at the start and shrink out at the end, so hide both ends inside a hood or machine.
 * SetRunning(false) brings the belt to a smooth stop.
 */
UCLASS(Blueprintable)
class VR_CONVEYORTRAINER_API AConveyorFlow : public AActor
{
	GENERATED_BODY()

public:
	AConveyorFlow();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Path start on the belt surface, in actor-local cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (MakeEditWidget = true))
	FVector PathStart = FVector(0.f, 100.f, 100.f);

	/** Path end on the belt surface, in actor-local cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (MakeEditWidget = true))
	FVector PathEnd = FVector(0.f, -100.f, 100.f);

	/** Belt speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	float Speed = 30.f;

	/** How fast the belt speeds up / slows down (cm/s per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	float Acceleration = 25.f;

	/** Distance between boxes in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	float Spacing = 60.f;

	/** Distance over which boxes grow in / shrink out at the ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	float FadeLength = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	FVector BoxSizeMin = FVector(26.f, 22.f, 14.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	FVector BoxSizeMax = FVector(42.f, 36.f, 30.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	int32 Seed = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	bool bRunning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	TObjectPtr<UMaterialInterface> BoxMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	TObjectPtr<UMaterialInterface> BoxMaterialAlt;

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void SetRunning(bool bInRunning);

	/** True while the belt is still moving (including slowing down). */
	UFUNCTION(BlueprintPure, Category = "Flow")
	bool IsMoving() const { return CurrentSpeed > 0.5f; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Flow")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Flow")
	TObjectPtr<UInstancedStaticMeshComponent> BoxesA;

	UPROPERTY(VisibleAnywhere, Category = "Flow")
	TObjectPtr<UInstancedStaticMeshComponent> BoxesB;

private:
	struct FBoxInfo
	{
		bool bAlt = false;
		int32 Instance = 0;
		FVector Size = FVector(30.f);
		float Yaw = 0.f;
		float Lateral = 0.f;
	};

	void BuildBoxes();
	void UpdateBoxes();

	TArray<FBoxInfo> Boxes;
	float Offset = 0.f;
	float CurrentSpeed = 0.f;
};
