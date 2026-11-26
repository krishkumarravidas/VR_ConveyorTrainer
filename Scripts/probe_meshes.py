"""Logs bounds (min/max, local cm) of the conveyor and machine meshes to Saved/probe_meshes.txt."""
import unreal

out = []
for path in ("/Game/Assests/Conveyor/SM_conveyor-long", "/Game/Assests/Conveyor/SM_machine"):
    sm = unreal.load_asset(path)
    b = sm.get_bounding_box()
    out.append(f"{path}: min={b.min} max={b.max}")
with open(unreal.Paths.project_saved_dir() + "probe_meshes.txt", "w") as f:
    f.write("\n".join(out))
