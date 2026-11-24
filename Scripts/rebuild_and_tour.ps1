# Rebuilds both trainer maps from Scripts/build_conveyor_level.py, then runs the desktop screenshot tour.
# Close the Unreal Editor first.
#   powershell -ExecutionPolicy Bypass -File Scripts\rebuild_and_tour.ps1 [-Map L_ConveyorTrainer_Playtest] [-SkipBuild]
param(
    [string]$Map = "L_ConveyorTrainer",
    [switch]$SkipBuild
)
$ErrorActionPreference = "Stop"
$proj = Split-Path -Parent $PSScriptRoot
$uproject = Join-Path $proj "VR_ConveyorTrainer.uproject"
$engine = "C:\Users\krish\Epic Games\UE_5.8\Engine\Binaries\Win64"

if (-not $SkipBuild) {
    $script = Join-Path $proj "Scripts\build_conveyor_level.py"
    Start-Process -FilePath "$engine\UnrealEditor-Cmd.exe" -ArgumentList "`"$uproject`" -run=pythonscript -script=`"$script`" -unattended -nosplash" -Wait -NoNewWindow -RedirectStandardOutput (Join-Path $proj "Saved\Logs\BuildLevel.log")
    Get-Content (Join-Path $proj "Saved\build_level.txt")
}

$shots = Join-Path $proj "Saved\Screenshots\WindowsEditor"
if (Test-Path $shots) { Get-ChildItem $shots -Filter "TrainerTour_*.png" | Remove-Item }
Start-Process -FilePath "$engine\UnrealEditor.exe" -ArgumentList "`"$uproject`" /Game/Assests/Maps/$Map -game -nohmd -windowed -ResX=1600 -ResY=900 -TrainerShots -nosplash -unattended" -Wait
Get-ChildItem $shots -Filter "TrainerTour_*.png" | Select-Object -ExpandProperty Name
