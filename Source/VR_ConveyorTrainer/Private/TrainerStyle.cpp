#include "TrainerStyle.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UMaterialInstanceDynamic* TrainerStyle::MakeColorMID(UObject* Outer, const FLinearColor& Color, bool bLit)
{
	const TCHAR* Paths[] = {
		bLit ? TEXT("/Game/Assests/MT/M_TrainerLit.M_TrainerLit") : TEXT("/Game/Assests/MT/M_TrainerUnlit.M_TrainerUnlit"),
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
	};
	for (const TCHAR* Path : Paths)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			return MID;
		}
	}
	return nullptr;
}

FString TrainerStyle::Wrap(const FString& In, int32 MaxChars)
{
	TArray<FString> Paragraphs;
	In.ParseIntoArray(Paragraphs, TEXT("\n"), false);

	FString Out;
	for (int32 P = 0; P < Paragraphs.Num(); ++P)
	{
		TArray<FString> Words;
		Paragraphs[P].ParseIntoArrayWS(Words);
		FString Line;
		for (const FString& Word : Words)
		{
			if (!Line.IsEmpty() && Line.Len() + 1 + Word.Len() > MaxChars)
			{
				Out += Line + TEXT("\n");
				Line.Reset();
			}
			Line += Line.IsEmpty() ? Word : TEXT(" ") + Word;
		}
		Out += Line.IsEmpty() ? FString(TEXT(" ")) : Line; // empty lines render oversized in TextRender
		if (P < Paragraphs.Num() - 1)
		{
			Out += TEXT("\n");
		}
	}
	return Out;
}
