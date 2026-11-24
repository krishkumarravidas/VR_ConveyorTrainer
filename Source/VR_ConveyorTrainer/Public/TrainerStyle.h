#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;

/**
 * One shared look: shop-floor prototype. Off-white boards with black text, grey steel,
 * safety-yellow tags, chunky coloured push-button caps. No glossy dark glass panels.
 */
namespace TrainerStyle
{
	// Boards and labels
	const FLinearColor Paper(0.78f, 0.77f, 0.72f);       // off-white whiteboard / paper
	const FLinearColor Ink(0.015f, 0.015f, 0.015f);      // near-black text
	const FLinearColor DimInk(0.18f, 0.18f, 0.17f);      // secondary text
	const FLinearColor Frame(0.06f, 0.065f, 0.07f);      // board frame, enclosures
	const FLinearColor Tape(0.85f, 0.78f, 0.45f);        // masking tape
	const FLinearColor Clipboard(0.28f, 0.17f, 0.08f);   // hardboard brown
	const FLinearColor Steel(0.25f, 0.26f, 0.27f);

	// State colours (readable on paper)
	const FLinearColor Accent(1.0f, 0.72f, 0.0f);        // safety yellow
	const FLinearColor Good(0.02f, 0.42f, 0.08f);        // dark green text
	const FLinearColor Warn(0.75f, 0.28f, 0.0f);         // dark orange text, never red flashing

	// Button caps
	const FLinearColor CapGreen(0.03f, 0.45f, 0.08f);
	const FLinearColor CapYellow(0.95f, 0.62f, 0.0f);
	const FLinearColor CapRed(0.6f, 0.03f, 0.02f);
	const FLinearColor CapLit(0.2f, 1.0f, 0.3f);         // toggled ON

	// Input visuals
	const FLinearColor Hand(0.85f, 0.88f, 0.95f);
	const FLinearColor Cursor(1.0f, 0.72f, 0.0f);

	/**
	 * Colour material. Unlit uses /Game/Assests/MT/M_TrainerUnlit, lit uses M_TrainerLit.
	 * Falls back to the engine basic-shape material.
	 */
	VR_CONVEYORTRAINER_API UMaterialInstanceDynamic* MakeColorMID(UObject* Outer, const FLinearColor& Color, bool bLit = false);

	/** Word-wraps text into lines of at most MaxChars (keeps existing line breaks). */
	VR_CONVEYORTRAINER_API FString Wrap(const FString& In, int32 MaxChars);
}
