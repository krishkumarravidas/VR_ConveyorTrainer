# L_ConveyorTrainer — Level Plan (v3, rebuilt 2026-11-24)

> **Status:** v3 is built and tested on desktop (all steps). It has not been tested on a Quest headset yet. Changes from v2 below:
> - **Factory bay:** 9 x 12 m, 5 m high, with racking, pallets, columns, lamps and a second running line.
> - **Moving boxes:** they ride Line 2 and stop when STOP MACHINE is pressed.
> - **Rollers:** 4 return idlers hang from brackets under the belt.
> - **Poke only:** no pinch and no laser. Everything is within a step of the start pad.
> - **UI:** restyled as physical shop-floor boards: a whiteboard, a clipboard, and push buttons on a control box.
> - **Playtest copy:** `L_ConveyorTrainer_Playtest` adds a stats board, a reset button, R/N keys and a CSV run log.
>
> The original v2 plan follows for reference. Where it conflicts with the notes above, the notes win.

Based on Project 02 brief (VR Conveyor Maintenance Trainer). Goal: a Quest user finishes in under 4 minutes with no developer help.

## Level
- **Name / location:** `/Game/Assests/Maps/L_ConveyorTrainer`, duplicated from `L_XRTemplate` so the VR pawn and teleport keep working.
- Set as Game Default Map and Editor Startup Map.
- Small enclosed workstation, about 6 m x 6 m. Everything sits in front of the player.

```
            [ back wall: "Simulation only" sign ]
   ┌───────────────────────────────────────────────┐
   │ [Checklist   ┌──────────────────────┐         │
   │  panel]      │ MOTOR ═══ BELT ═══ ◎ │         │  conveyor ~0.9 m high
   │              └──────────────────────┘         │  ◎ = damaged idler roller
   │ [Work order                    [Instruction   │
   │  panel]                         panel]        │
   │            ░░ teleport floor ░░               │
   │                 ▲ player start                │
   └───────────────────────────────────────────────┘
```

- **Player start:** about 2.5 m from the conveyor, facing it.
- **Conveyor:** `SM_conveyor-long`, left to right. `SM_machine` = motor housing at the left end.
- **Idler roller:** engine cylinder under the belt. One has `M_Damaged`: this is the single fault.
- **Labels:** a small floating label above each part, shown only while it is highlighted.
- **Movement:** teleport only on a small marked floor. No smooth movement.

## Instruction system (new)
One main **instruction panel** that always says what to do *now*. It is the single source of guidance.
- **Position:** fixed, front-right of the conveyor, at eye height (~1.5 m), angled toward the player.
- **Content per step:** a title (e.g. "Step 3 of 5 — Identify parts"), one short sentence of what to do, a progress bar (5 dots), and a "how" line that changes to match the input being used:
  - Hands: "Point your hand and pinch to select" / "Poke the button"
  - Controllers: "Point and pull the trigger"
- **Welcome screen (step 0):** shows the project name, then "Simulation only — not a real lockout procedure or safety certification", then a 3-line controls guide (hands + controllers). Ends with a big **Start** button.
- **Feedback:** short text with a coloured edge on the panel. Green = correct, amber = try again, with one-line hint. No red flashing.
- **Audio:** a soft click on select and a chime on step complete (optional if time).

### Clean UI rules
- One visual style for every panel: dark background at about 85% opacity, white text, one accent colour (safety yellow) for the current item.
- Text at least 1.5 cm tall at about 1.5 m (roughly 40–48 px on a 1 m wide widget). Max 2 lines of body text per step.
- Buttons are at least 6 cm x 6 cm (comfortable to poke with a fingertip), spaced at least 2 cm apart.
- Only the panel for the current step is fully visible. The others fade to 30% or hide.
- Nothing closer than 0.5 m or farther than 3 m from the player.

## Hand tracking (must-have)
Hands and controllers both work, and the user can switch at any time.
- **Already on:** the `OpenXRHandTracking` plugin is enabled in the .uproject.
- **Hand visuals:** tracked hand meshes, or simple joint spheres as a fallback, driven by OpenXR hand joint data.
- **Interactions:**
  - **Poke:** fingertip touch on UI buttons (Start, checklist ticks, diagnosis choices). Uses a WidgetInteraction component on the index tip, plus a small collision sphere for parts.
  - **Pinch-ray:** a ray from the hand along the pointing direction. Index–thumb pinch = select. Used to select conveyor parts from a distance.
  - Same `Select()` path as the controller trigger, so step logic does not care about input type.
- **Auto-switch:** detect whether hands or controllers are tracked and swap visuals plus the instruction "how" line.
- **To verify tomorrow:**
  - The Quest Android manifest needs the hand-tracking permission/feature (`com.oculus.permission.HAND_TRACKING`, `oculus.software.handtracking`). This is not in `DefaultEngine.ini` yet.
  - Check whether the 5.8 VR Template pawn already has hand support, or whether we add it.
  - Test pinch reliability on the Quest 2 and Quest 3.
- **Fallback:** if pinch is unreliable on device, keep poke for all UI and move part selection to poke too, with parts within reach.

## Step flow
0. **Welcome:** read the intro and controls, then press **Start**.
1. **Work order:** "Inspect the reported abnormal condition." Press **Accept**.
2. **Checklist (simulated):** tick "Machine stopped" and "Isolated". Labelled as simulated and instructor-reviewed.
3. **Identify parts:** select the belt, then the idler roller, then the motor housing. Each highlights (`M_Highlight`) and shows its label. The step is done after all 3.
4. **Inspect:** close-up hint near the damaged roller. "Look closely at the rollers."
5. **Diagnose:** pick the faulty part (3 buttons), then a response, e.g. "Report & escalate to qualified maintenance". Show feedback, then the **completion summary** (checklist all ticked, time taken) with **Restart**.

Logic: `BP_StepManager` drives steps. Parts use `AConveyorPart` (C++) / `BP_Part`.

## Quest performance
- Mobile forward shading, baked lighting, no Lumen. One directional light plus 1–2 stationary lights.
- Low draw calls and plain materials. Keep 72+ fps on the Quest 2.

## Out of scope (per brief)
No belt physics, disassembly, repair animation, electrical system, torque specs, or claims of real safety certification.

## Build approach
1. Editor Python script places meshes, panels, lights, and player start (check that the Python Editor Script plugin is enabled).
2. Instruction panel widget + step flow in Blueprint.
3. Hand tracking: manifest settings, hand visuals, poke + pinch-ray.
4. Package to Quest, then test with hands and with controllers.
