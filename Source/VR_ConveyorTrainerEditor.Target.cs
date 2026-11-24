using UnrealBuildTool;

public class VR_ConveyorTrainerEditorTarget : TargetRules
{
	public VR_ConveyorTrainerEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("VR_ConveyorTrainer");
	}
}
