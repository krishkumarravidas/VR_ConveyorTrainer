# VR Conveyor Belt Inspection & Fault-Diagnosis Trainer

A VR prototype for Meta Quest that walks a novice through the basic parts of a conveyor system and a guided fault-identification workflow. Built for the AR/VR Tech Team 7-day industry showcase (Project 02).

> **Simulation only.** This is procedural familiarization. It is not a real lockout/tagout procedure and not a safety certification or training accreditation.

## Problem
New maintenance technicians often meet real equipment before they can name its parts or follow a fault-reporting workflow. This prototype lets a trainee practise that workflow safely in VR first: identify the main conveyor components, spot an obvious fault, and choose a sensible high-level response (report and escalate to qualified maintenance).

## Target user
Novice or trainee maintenance technicians in manufacturing and facilities settings, and the lab funder evaluating VR training use cases.

## User journey
1. Enter a small workstation with a stationary conveyor and a floating work order.
2. Confirm the (simulated) machine is stopped and isolated using a short generic checklist.
3. Point at three highlighted components: belt, idler roller, motor housing. A label explains each.
4. Inspect one intentionally obvious defect (a damaged roller).
5. Choose the faulty component and a high-level response. See feedback and a completion summary.

## Implemented features (working now)
- Unreal Engine 5.8 Blueprint project based on the VR Template
- Conveyor scene built from low-poly assets (belt, motor housing, roller)
- Step manager Blueprint that shows an on-screen instruction for each of the five steps
- Desktop debug control to advance steps (see Controls)
- Android (Meta Quest) packaging works; a template build was packaged successfully

## Planned features (not done yet)
- Selectable, highlightable parts with labels (belt, roller, motor housing)
- Controller pointer selection in VR
- Safety checklist screen (simulation only)
- Fault inspection, diagnosis choice and feedback
- Completion summary and restart
- Full on-headset test and recorded demo video

## Controls
- **Desktop debug (current):** press **N** to advance to the next step.
- **Meta Quest 2/3 (planned):** point with the controller ray and use the trigger to select.

## Assets & licences
- Kenney Factory Kit 3.0 by Kenney (kenney.nl), CC0 1.0. Licence text: `Licenses/Kenney_FactoryKit_License.txt`
- Unreal Engine VR Template content, used under the Unreal Engine licence.

## Limitations
- Simulation only. Not a real lockout/tagout procedure and not a safety certification.
- One scene, one fault, one diagnosis decision.
- No mechanical simulation, repair animation, torque specifications or electrical system.
- Tested on desktop; Quest device testing is still in progress.

## Setup
- Unreal Engine 5.8, Blueprint-only project
- Target: Meta Quest 2/3 via Android/OpenXR (Mobile Forward rendering)
- Android SDK/NDK installed through Unreal's Turnkey (NDK r27c)
- **Install Git LFS** (`git lfs install`) before cloning, because the Unreal assets are stored in LFS.
- Open `VR_ConveyorTrainer.uproject` with UE 5.8.

## Next steps
- Add part highlighting and controller selection
- Add the checklist, fault and diagnosis steps and the completion screen
- Test on a Quest headset, record the backup demo video, and finalise this README
- Possible later work: more faults, a second scenario, data logging
