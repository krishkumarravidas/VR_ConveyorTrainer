using UnrealBuildTool;

public class VR_ConveyorTrainerTarget : TargetRules
{
	public VR_ConveyorTrainerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("VR_ConveyorTrainer");
	}
}
