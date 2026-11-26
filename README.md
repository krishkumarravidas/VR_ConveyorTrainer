# VR Conveyor Belt Inspection & Fault-Diagnosis Trainer

A VR prototype for Meta Quest that walks a novice through the basic parts of a conveyor system and a guided fault-identification workflow. Built for the AR/VR Tech Team 7-day industry showcase (Project 02).

> **Simulation only.** This is procedural familiarization. It is not a real lockout/tagout procedure and not a safety certification or training accreditation.

## Problem
New maintenance technicians often meet real equipment before they can name its parts or follow a fault-reporting workflow. This prototype lets a trainee practise that workflow safely in VR first: identify the main conveyor components, spot an obvious fault, and choose a sensible high-level response (report and escalate to qualified maintenance).

## Target user
Novice or trainee maintenance technicians in manufacturing and facilities settings, and the lab funder evaluating VR training use cases.

## User journey
1. Stand on the green pad in a small factory bay. Line 2 is running with boxes on it; Line 1 runs in the background.
2. Read the work order on the clipboard and accept it on the control box.
3. Press STOP MACHINE (the belt slows to a stop) and ISOLATE POWER. Both are simulated.
4. Touch three tagged parts: belt, idler roller, motor housing. A tag explains each one.
5. Inspect the belt, the rollers under it and the motor housing, then touch the damaged one. The fault is picked at random every round.
6. Choose the faulty part and the right response (report to maintenance). See the summary.

## Implemented features (working now, tested on desktop)
- **Factory bay:** steel columns with hazard bases, pallet racking, pallets, an MCC cabinet, pipes, hanging lamps and painted floor zones.
- **Line 2 (under inspection):** a conveyor on a stand with yellow frame rails and an infeed hood with a strip curtain. Cardboard boxes ride the belt and stop when you press STOP MACHINE.
- **Rollers:** four idler rollers sit in a straight line right under the belt, with their axles in the yellow frame rails. The ends stick out so you can see them from standing height.
- **Motor housing:** sits on a steel plinth, so the raised belt feeds straight into its opening.
- **Random fault every round:** belt, idler roller or motor housing (never the same one twice in a row). Each has its own visible damage:
  - Belt: a torn, fraying patch at the front edge.
  - Roller: cracked, skewed 6 degrees and dropped at the front.
  - Motor: an oil leak down the plinth into a puddle.
  The healthy parts look normal and their inspection tags say so.
- **Line 1:** a second, always-running conveyor in the background.
- **Full 5-step flow:**
  - Welcome
  - Work order
  - Simulated stop/isolate checklist
  - Identify 3 parts
  - Inspect to find the fault
  - Diagnose the part and choose a response
  - Summary and Restart
- **Prototype-style UI:**
  - Instructions on an off-white whiteboard with masking tape, a "STEP n/5" tag and a coloured status strip (yellow = working, green = correct, orange = try again).
  - Work order on a clipboard that ticks off as you go.
  - Chunky industrial push buttons (coloured cap and label strip) on a "LINE 2 CONTROL" box.
  - Parts carry yellow hanging tags with a string to the part.
- **Poke-only input:**
  - Hands: touch with your index fingertip.
  - Controllers: touch with a yellow ball on the controller tip.
  - Things light up just before you touch them.
  - No pinch, no laser.
- **Desktop playtest controls (no headset):** WASD to walk, hold right mouse to look, left-click to touch anything within arm's reach.
- **Playtest level `L_ConveyorTrainer_Playtest`:** the same level plus:
  - a live stats board (timer against the 4-minute target, current step, input, retries, belt state)
  - a red RESET RUN button
  - PC keys R (restart) and N (skip step)
  - one CSV row per finished run in `Saved/Playtest/playtest_runs.csv`, with the fault, per-step seconds and pass/fail
  - the board also shows which part is faulty this round, for the person running the test

![Welcome: factory bay, Line 2 running](Docs/Screenshots/01_welcome_factory.png)
![Checklist on the control box (belt stopping)](Docs/Screenshots/02_control_box_checklist.png)
![Inspecting the damaged idler roller](Docs/Screenshots/03_inspect_roller.png)
![Completion summary](Docs/Screenshots/04_complete.png)
![Playtest board and clipboard](Docs/Screenshots/05_playtest_board.png)
![Fault: torn belt](Docs/Screenshots/06_fault_belt.png)
![Fault: oil leak under the motor housing](Docs/Screenshots/07_fault_motor.png)
![Motor housing on its plinth, belt feeding into it](Docs/Screenshots/08_machine_plinth.png)

## Planned / not verified yet
- A full test on the headset (Quest 2/3), with hands and with controllers, plus the recorded backup demo video
- Instructor review of the simulated checklist wording

## Controls
- **Hands (Quest):** touch buttons and parts with your index fingertip. Pull your finger back out to press again.
- **Controllers (Quest 2/3):** touch things with the yellow ball in front of the controller. Teleport with the thumbstick (VR Template).
- **Desktop (no headset):**
  - WASD to walk
  - Hold right mouse to look around
  - Aim the dot and left-click to touch. The dot turns yellow when something is in reach.

## Playtesting
- Open `L_ConveyorTrainer_Playtest` in the editor:
  - Press Play (Selected Viewport) to test on desktop.
  - Use VR Preview to test with a Quest Link headset.
- Each finished run adds a line to `Saved/Playtest/playtest_runs.csv`. Use it to check the "under 4 minutes" acceptance test across testers.

## Assets & licences
- Kenney Factory Kit 3.0 by Kenney (kenney.nl), CC0 1.0. Licence text: `Licenses/Kenney_FactoryKit_License.txt`
- Unreal Engine VR Template content, used under the Unreal Engine licence.

## Limitations
- Simulation only. Not a real lockout/tagout procedure and not a safety certification.
- One scene, one fault, one diagnosis decision.
- No mechanical simulation, repair animation, torque specifications or electrical system.
- Tested on desktop only so far. Fingertip poke, controller-tip poke and the hand-tracking manifest entries still need a Quest test.
- Parts must be reached physically (poke only). The roller ends are at about 55 cm, so trainees need to bend down a little. Check that this is comfortable on device.
- About 170 separate mesh actors and 4 movable lights. Check frame rate on a Quest 2 and merge meshes or bake lighting if needed.
- Step logic is written in C++ and exposed to Blueprint (events, dispatchers and editable properties), rather than drawn as a Blueprint graph.
- Lights are movable (no baked lighting yet).

## Setup
- Unreal Engine 5.8, C++ + Blueprint project (Visual Studio 2022+ with the "Game development with C++" workload to compile)
- Target: Meta Quest 2/3 via Android/OpenXR (Mobile Forward rendering)
- Android SDK/NDK installed through Unreal's Turnkey (NDK r27c)
- **Install Git LFS** (`git lfs install`) before cloning, because the Unreal assets are stored in LFS.
- Open `VR_ConveyorTrainer.uproject` with UE 5.8 and let it build the module.
- Rebuild both maps from script (optional, editor closed): `UnrealEditor-Cmd.exe VR_ConveyorTrainer.uproject -run=pythonscript -script=Scripts/build_conveyor_level.py`
- Rebuild and take a screenshot of every step in one go: `powershell -ExecutionPolicy Bypass -File Scripts\rebuild_and_tour.ps1` (add `-SkipBuild`, `-Map L_ConveyorTrainer_Playtest`, or `-Fault 0|1|2` to force the belt, roller or motor fault). Images land in `Saved/Screenshots/WindowsEditor`.

## Code overview
- `TrainerStepManager`: the 5-step flow, random fault per round, feedback, summary, belt stop, and playtest stats/log
- `XRHandInteractor`: poke-only input for hands, controller tips and desktop click; one touch path for all three
- `ConveyorPart`: touchable part with a padded touch zone, hanging tag, highlight, and a healthy/faulty state (materials, position, extra fault props, inspection note)
- `ConveyorFlow`: instanced cardboard boxes riding a belt path, with a smooth stop and start
- `TrainerButton` / `TrainerPanel`: industrial push buttons and whiteboard / clipboard / sign boards

## Next steps
- Package to Quest and test hands and controllers. Tune poke radius, text size and the roller height on the device.
- Record the backup demo video and finalise this README
- Have the instructor review the checklist wording
- Possible later work: more faults, a second scenario, data logging
