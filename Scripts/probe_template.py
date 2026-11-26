# Prints what is in the VR template level and the sizes of the conveyor meshes.
import unreal

out = []
def log(*a):
    out.append(" ".join(str(x) for x in a))

unreal.EditorLoadingAndSavingUtils.load_map("/Game/XRFramework/Levels/L_XRTemplate")
world = unreal.EditorLevelLibrary.get_editor_world()
log("WORLD", world.get_path_name())
ws = world.get_world_settings()
log("GAMEMODE", ws.get_editor_property("default_game_mode"))
for lvl in unreal.EditorLevelUtils.get_levels(world):
    log("LEVEL", lvl.get_path_name())
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    t = a.get_actor_transform()
    o, e = a.get_actor_bounds(False)
    log("ACTOR", a.get_class().get_name(), a.get_actor_label(), t.translation, t.rotation.rotator(), t.scale3d, "ext", e)

for p in ["/Game/Assests/Conveyor/SM_conveyor-long", "/Game/Assests/Conveyor/SM_machine"]:
    m = unreal.load_asset(p)
    b = m.get_bounds()
    log("MESH", p, "origin", b.origin, "extent", b.box_extent, "slots", len(m.static_materials))
for p in ["/Game/Assests/MT/M_Highlight", "/Game/Assests/MT/M_Damaged", "/Game/Assests/Conveyor/M_colormap1"]:
    m = unreal.load_asset(p)
    log("MAT", p, m.get_class().get_name() if m else None)

with open(unreal.Paths.project_saved_dir() + "probe.txt", "w") as f:
    f.write("\n".join(out))
