"""
Builds two maps from scratch (safe to re-run: both are replaced):
  /Game/Assests/Maps/L_ConveyorTrainer           - the trainer
  /Game/Assests/Maps/L_ConveyorTrainer_Playtest  - same level plus a playtest board, reset button and run log

Run headless (editor closed):
  UnrealEditor-Cmd.exe VR_ConveyorTrainer.uproject -run=pythonscript -script=Scripts/build_conveyor_level.py

Coordinates (cm): player stands at the origin facing +X. +Y is to the player's right. Floor top is Z = 0.
Everything the trainee must touch is within a step of the start spot (poke-only input).
"""
import math
import unreal

MAPS = "/Game/Assests/Maps"
EXPOSURE_BIAS = 0.7
MT = "/Game/Assests/MT"
LOG = []


def log(*a):
    msg = " ".join(str(x) for x in a)
    LOG.append(msg)
    unreal.log("[ConveyorLevel] " + msg)


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder")
CONVEYOR = unreal.load_asset("/Game/Assests/Conveyor/SM_conveyor-long")
MACHINE = unreal.load_asset("/Game/Assests/Conveyor/SM_machine")
M_HIGHLIGHT = unreal.load_asset(MT + "/M_Highlight")
M_DAMAGED = unreal.load_asset(MT + "/M_Damaged")


# ---------------------------------------------------------------- materials
def make_color_material(name, unlit):
    path = f"{MT}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset(name, MT, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    if unlit:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        mel.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
        rough.set_editor_property("r", 0.75)
        mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    log("created material", path)
    return mat


def make_instance(name, parent, rgb):
    path = f"{MT}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = asset_tools.create_asset(name, MT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", parent)
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1))
    mel.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi


M_UNLIT = make_color_material("M_TrainerUnlit", True)
M_LIT = make_color_material("M_TrainerLit", False)


def ensure_instanced_usage(mat):
    """Boxes and hand joints are instanced meshes; without this flag the game falls back to the checker material."""
    if not mat.get_editor_property("used_with_instanced_static_meshes"):
        mat.set_editor_property("used_with_instanced_static_meshes", True)
        mel.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
        log("enabled instanced usage on", mat.get_name())


ensure_instanced_usage(M_UNLIT)
ensure_instanced_usage(M_LIT)
MI_FLOOR = make_instance("MI_Floor", M_LIT, (0.16, 0.16, 0.15))          # sealed concrete
MI_WALL = make_instance("MI_Wall", M_LIT, (0.42, 0.42, 0.40))            # painted block, upper
MI_WALL_LOW = make_instance("MI_WallLower", M_LIT, (0.10, 0.17, 0.15))   # dark green dado band
MI_CEILING = make_instance("MI_Ceiling", M_LIT, (0.06, 0.06, 0.065))
MI_STEEL = make_instance("MI_Steel", M_LIT, (0.12, 0.13, 0.15))
MI_ROLLER = make_instance("MI_Roller", M_LIT, (0.45, 0.47, 0.5))
MI_STRIPE = make_instance("MI_SafetyStripe", M_UNLIT, (0.75, 0.52, 0.0))
MI_YELLOW = make_instance("MI_SafetyYellow", M_LIT, (0.80, 0.55, 0.02))
MI_BLACK = make_instance("MI_Black", M_LIT, (0.02, 0.02, 0.02))
MI_RACK_ORANGE = make_instance("MI_RackOrange", M_LIT, (0.75, 0.22, 0.02))
MI_RACK_BLUE = make_instance("MI_RackBlue", M_LIT, (0.03, 0.09, 0.40))
MI_PALLET = make_instance("MI_Pallet", M_LIT, (0.33, 0.21, 0.10))
MI_CARDBOARD = make_instance("MI_Cardboard", M_LIT, (0.42, 0.27, 0.12))
MI_CARDBOARD_DARK = make_instance("MI_CardboardDark", M_LIT, (0.30, 0.19, 0.08))
MI_BELT = make_instance("MI_BeltRubber", M_LIT, (0.035, 0.035, 0.035))
MI_RED = make_instance("MI_Red", M_LIT, (0.55, 0.03, 0.02))
MI_GREEN = make_instance("MI_FloorGreen", M_LIT, (0.04, 0.28, 0.10))
MI_GLOW = make_instance("MI_LampGlow", M_UNLIT, (1.0, 0.93, 0.80))
MI_CURTAIN = make_instance("MI_StripCurtain", M_LIT, (0.05, 0.07, 0.08))
MI_CABINET = make_instance("MI_Cabinet", M_LIT, (0.30, 0.32, 0.31))
MI_MACHINE = make_instance("MI_MachineGreen", M_LIT, (0.10, 0.20, 0.14))   # machine-enamel green


def V(x, y, z):
    return unreal.Vector(x, y, z)


def R(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def face_player(x, y):
    """Yaw so the actor's +X points at the player (origin)."""
    return math.degrees(math.atan2(-y, -x))


def spawn(cls, loc, rot=None, label=None):
    a = actors.spawn_actor_from_class(cls, loc, rot or R())
    if label:
        a.set_actor_label(label)
    return a


def mesh(label, sm, center, scale, mat, rot=None, collide=True, folder="Environment", shadow=True):
    a = spawn(unreal.StaticMeshActor, center, rot, label)
    c = a.static_mesh_component
    c.set_static_mesh(sm)
    c.set_material(0, mat)
    a.set_actor_scale3d(scale)
    if not collide:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if not shadow:
        c.set_cast_shadow(False)
    a.set_folder_path(folder)
    return a


def box(label, center, size, mat, rot=None, collide=True, folder="Environment", shadow=True):
    return mesh(label, CUBE, center, V(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0), mat, rot, collide, folder, shadow)


def cyl(label, center, diameter, length, mat, rot=None, collide=False, folder="Environment"):
    """Engine cylinder: axis along local Z."""
    return mesh(label, CYLINDER, center, V(diameter / 100.0, diameter / 100.0, length / 100.0), mat, rot, collide, folder)


def text(label, loc, yaw, words, size, color=(0, 0, 0), folder="Environment/Signs", pitch=0.0):
    a = spawn(unreal.TextRenderActor, loc, R(pitch=pitch, yaw=yaw), label)
    t = a.text_render
    t.set_text(words)
    t.set_world_size(size)
    t.set_text_render_color(unreal.Color(r=int(color[0] * 255), g=int(color[1] * 255), b=int(color[2] * 255), a=255))
    try:
        t.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        t.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    except Exception as e:  # enum names differ between versions
        log("text alignment not set:", e)
    a.set_folder_path(folder)
    return a


def lateral(yaw_deg):
    """Unit vector along an actor's local +Y for a given yaw."""
    y = math.radians(yaw_deg)
    return V(-math.sin(y), math.cos(y), 0)


def panel(label, loc, width, height, title, body, footer, style, body_size=3.2, title_size=4.6, footer_size=2.4,
          yaw=None, pitch=0.0, folder="UI"):
    yaw = face_player(loc.x, loc.y) if yaw is None else yaw
    p = spawn(unreal.TrainerPanel, loc, R(pitch=pitch, yaw=yaw), label)
    p.set_editor_property("style", style)
    p.set_editor_property("width", width)
    p.set_editor_property("height", height)
    p.set_editor_property("body_size", body_size)
    p.set_editor_property("title_size", title_size)
    p.set_editor_property("footer_size", footer_size)
    p.set_editor_property("title", title)
    p.set_editor_property("body", body)
    p.set_editor_property("footer", footer)
    p.set_folder_path(folder)
    return p


def board_legs(label, p, width, bottom_z, folder="UI"):
    """Two steel posts from the floor to the bottom of a board."""
    yaw = p.get_actor_rotation().yaw
    side = lateral(yaw)
    back = unreal.MathLibrary.get_forward_vector(R(yaw=yaw)) * -2.0
    c = p.get_actor_location()
    for i, s in enumerate((-1, 1)):
        o = c + side * (s * (width * 0.5 - 6)) + back
        box(f"{label}_Leg{i}", V(o.x, o.y, bottom_z * 0.5), (4, 4, bottom_z), MI_STEEL, rot=R(yaw=yaw), folder=folder)


def world_to_local_offset(actor, world_offset):
    t = actor.get_actor_transform()
    return unreal.MathLibrary.inverse_transform_location(t, actor.get_actor_location() + world_offset)


def make_part(label, sm, loc, rot, scale, name, role, note, faulty, label_world_offset, overrides=None):
    p = spawn(unreal.ConveyorPart, loc, rot, label)
    p.set_actor_scale3d(scale)
    p.set_editor_property("part_mesh", sm)
    p.set_editor_property("highlight_material", M_HIGHLIGHT)
    p.set_editor_property("part_name", name)
    p.set_editor_property("description", role)
    p.set_editor_property("inspect_note", note)
    p.set_editor_property("is_faulty", faulty)
    if overrides:
        p.set_editor_property("material_overrides", overrides)
    p.set_editor_property("label_offset", world_to_local_offset(p, label_world_offset))
    p.set_folder_path("Conveyor/Parts")
    return p


def point_light(label, loc, intensity, radius, shadows):
    l = spawn(unreal.PointLight, loc, None, label)
    c = l.point_light_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_editor_property("intensity", intensity)
    c.set_editor_property("attenuation_radius", radius)
    c.set_editor_property("cast_shadows", shadows)
    c.set_editor_property("light_color", unreal.Color(r=255, g=240, b=220, a=255))
    l.set_folder_path("Lighting")
    return l


def hanging_lamp(label, x, y, z_lamp, ceiling, intensity, radius, shadows):
    cyl(f"{label}_Rod", V(x, y, (z_lamp + ceiling) / 2), 2, ceiling - z_lamp, MI_STEEL, folder="Lighting")
    cyl(f"{label}_Shade", V(x, y, z_lamp + 6), 55, 12, MI_STEEL, folder="Lighting")
    cyl(f"{label}_Glow", V(x, y, z_lamp - 0.5), 46, 1, MI_GLOW, folder="Lighting")
    return point_light(label, V(x, y, z_lamp - 20), intensity, radius, shadows)


def flow(label, start, end, speed, spacing, seed):
    f = spawn(unreal.ConveyorFlow, V(0, 0, 0), None, label)
    f.set_editor_property("path_start", start)
    f.set_editor_property("path_end", end)
    f.set_editor_property("speed", speed)
    f.set_editor_property("spacing", spacing)
    f.set_editor_property("seed", seed)
    f.set_editor_property("box_material", MI_CARDBOARD)
    f.set_editor_property("box_material_alt", MI_CARDBOARD_DARK)
    f.set_folder_path("Conveyor/Flow")
    return f


def box_stack(label, x, y, yaw, layers, folder="Environment/Props"):
    """Pallet with a few cardboard boxes on it."""
    box(f"{label}_Pallet", V(x, y, 7), (120, 100, 14), MI_PALLET, rot=R(yaw=yaw), folder=folder)
    z = 14
    for i in range(layers):
        h = 38 - i * 4
        mat = MI_CARDBOARD if i % 2 == 0 else MI_CARDBOARD_DARK
        box(f"{label}_Boxes{i}", V(x, y, z + h / 2), (112 - i * 10, 92 - i * 8, h), mat, rot=R(yaw=yaw + (i % 2) * 3), folder=folder)
        z += h


# ================================================================ level builder
def build(level_path, playtest):
    if unreal.EditorAssetLibrary.does_asset_exist(level_path):
        unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
        unreal.EditorAssetLibrary.delete_asset(level_path)
        log("deleted old", level_path)

    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    unreal.EditorLoadingAndSavingUtils.save_map(world, level_path)
    world = unreal.EditorLevelLibrary.get_editor_world()
    ws = world.get_world_settings()
    ws.set_editor_property("default_game_mode", unreal.load_class(None, "/Game/XRFramework/Blueprints/BP_XRGameMode.BP_XRGameMode_C"))

    # ------------------------------------------------------------ factory bay (9 m x 12 m, 5 m high)
    X0, X1, Y0, Y1, H = -250.0, 650.0, -600.0, 600.0, 500.0
    cx, cy = (X0 + X1) / 2, (Y0 + Y1) / 2
    box("Floor", V(cx, cy, -5), (X1 - X0, Y1 - Y0, 10), MI_FLOOR)
    box("Ceiling", V(cx, cy, H + 5), (X1 - X0, Y1 - Y0, 10), MI_CEILING, collide=False)
    DADO = 120.0
    for name, c, s in (
        ("Back", V(X1 + 5, cy, 0), (10, Y1 - Y0, 0)),
        ("Front", V(X0 - 5, cy, 0), (10, Y1 - Y0, 0)),
        ("Left", V(cx, Y0 - 5, 0), (X1 - X0, 10, 0)),
        ("Right", V(cx, Y1 + 5, 0), (X1 - X0, 10, 0)),
    ):
        box(f"Wall_{name}", V(c.x, c.y, (H + DADO) / 2), (s[0], s[1], H - DADO), MI_WALL)
        box(f"Wall_{name}_Lower", V(c.x, c.y, DADO / 2), (s[0], s[1], DADO), MI_WALL_LOW)

    # Roof trusses and service pipes.
    for i, x in enumerate((0.0, 300.0, 600.0)):
        box(f"RoofBeam_{i}", V(x, cy, H - 25), (20, Y1 - Y0, 30), MI_STEEL, collide=False, folder="Environment/Structure")
    for i, (y, d) in enumerate(((Y1 - 12, 14), (Y1 - 34, 9))):
        cyl(f"Pipe_{i}", V(cx, y, 420 - i * 18), d, X1 - X0, MI_STEEL if i else MI_RED, rot=R(pitch=90), folder="Environment/Structure")

    # Steel columns with yellow/black hazard bases.
    for i, (x, y) in enumerate(((-100, -330), (-100, 330), (350, -330), (350, 330))):
        box(f"Column_{i}", V(x, y, H / 2), (30, 30, H), MI_STEEL, folder="Environment/Structure")
        for k in range(4):
            box(f"Column_{i}_Guard{k}", V(x, y, 12.5 + k * 25), (34, 34, 25), MI_YELLOW if k % 2 == 0 else MI_BLACK,
                folder="Environment/Structure")

    # ------------------------------------------------------------ Line 2: the conveyor under inspection
    # SM_conveyor-long: 200 long (mesh X) x 100 wide x 40 tall, pivot bottom centre.
    # Runs left-right across the player's view on a 60 cm stand: belt top at 100 cm, front edge 75 cm from the player.
    CONV_X, CONV_Y, STAND_H = 125.0, 40.0, 60.0
    CONV_LEN, CONV_W = 200.0, 100.0
    BELT_TOP = STAND_H + 40.0
    FRONT_X, BACK_X = CONV_X - CONV_W / 2, CONV_X + CONV_W / 2
    F = "Conveyor"

    for i, dy in enumerate((-CONV_LEN / 2 + 8, CONV_LEN / 2 - 8)):
        box(f"Stand_Leg_{i}", V(CONV_X, CONV_Y + dy, STAND_H / 2), (CONV_W - 10, 8, STAND_H), MI_STEEL, folder=F)
        box(f"Stand_Foot_{i}", V(CONV_X, CONV_Y + dy, 1.5), (CONV_W + 6, 14, 3), MI_STEEL, folder=F)
    # Side frame rails under the belt; roller brackets hang from these.
    for i, x in enumerate((FRONT_X - 1.5, BACK_X + 1.5)):
        box(f"Frame_Rail_{i}", V(x, CONV_Y, STAND_H - 3), (3, CONV_LEN, 6), MI_YELLOW, folder=F)
    box("Stand_Brace", V(CONV_X, CONV_Y, 14), (6, CONV_LEN - 16, 6), MI_STEEL, folder=F)

    # Return idlers: 4 rollers hanging 14 cm under the belt, evenly spaced. Index 1 (straight ahead) is the fault.
    ROLLER_Z, ROLLER_D, ROLLER_LEN = STAND_H - 14.0, 10.0, CONV_W + 10.0
    ROLLER_Y = [CONV_Y + o for o in (-75.0, -25.0, 25.0, 75.0)]
    FAULT = 1
    for i, y in enumerate(ROLLER_Y):
        for j, x in enumerate((FRONT_X - 1.5, BACK_X + 1.5)):
            drop = 4.0 if (i == FAULT and j == 0) else 0.0          # fault: bracket bent, front end dropped
            h = (STAND_H - 6) - (ROLLER_Z - 4 - drop)
            box(f"Roller_{i}_Bracket{j}", V(x, y, (STAND_H - 6) - h / 2), (2.5, 6, h), MI_STEEL, collide=False, folder=F)
        if i == FAULT:
            continue
        cyl(f"Roller_{i}", V(CONV_X, y, ROLLER_Z), ROLLER_D, ROLLER_LEN, MI_ROLLER, rot=R(pitch=90), folder=F)

    # Belt (dark rubber top), damaged idler roller (skewed + dropped one end), motor / drive housing.
    slots = CONVEYOR.get_editor_property("static_materials")
    log("conveyor material slots:", len(slots), [str(s.material_slot_name) for s in slots])
    belt = make_part(
        "Part_Belt", CONVEYOR, V(CONV_X, CONV_Y, STAND_H), R(yaw=90), V(1, 1, 1),
        "Belt", "Carries product along the line.", "Belt surface and tracking look OK.", False,
        V(-25, 55, 62), overrides=[MI_BELT])

    roller = make_part(
        "Part_IdlerRoller", CYLINDER, V(CONV_X, ROLLER_Y[FAULT], ROLLER_Z - 2),
        R(pitch=94, yaw=7),  # axis along X, skewed 7 deg and dropped at the front: visibly out of line
        V(ROLLER_D / 100, ROLLER_D / 100, ROLLER_LEN / 100),
        "Idler roller", "Supports the return belt and keeps it tracking straight.",
        "Cracked and out of line.", True,
        V(-72, 0, 30), overrides=[M_DAMAGED])

    motor = make_part(
        "Part_MotorHousing", MACHINE, V(CONV_X + 5, CONV_Y - CONV_LEN / 2 - 62, 0), R(yaw=90), V(1, 1, 1),
        "Motor housing", "Covers the drive motor that powers the belt.", "Housing intact. No damage.", False,
        V(-45, 20, 150), overrides=[MI_MACHINE])

    # Infeed hood with strip curtain at the right-hand end: boxes appear from inside it.
    HY0, HY1 = CONV_Y + CONV_LEN / 2 - 30, CONV_Y + CONV_LEN / 2 + 5
    hy = (HY0 + HY1) / 2
    box("Hood_Top", V(CONV_X, hy, BELT_TOP + 52), (CONV_W + 12, HY1 - HY0, 4), MI_YELLOW, folder=F)
    for i, x in enumerate((FRONT_X - 4, BACK_X + 4)):
        box(f"Hood_Side_{i}", V(x, hy, BELT_TOP + 25), (4, HY1 - HY0, 54), MI_YELLOW, folder=F)
    for k in range(6):
        box(f"Hood_Curtain_{k}", V(FRONT_X + 8 + k * 16.8, HY0 + 1, BELT_TOP + 24), (15, 1, 48), MI_CURTAIN,
            collide=False, folder=F)
    text("Sign_Line2", V(FRONT_X - 6.5, hy, BELT_TOP + 38), 180, "LINE 2", 7.0, folder=F)
    text("Sign_Bay4", V(FRONT_X - 6.5, hy, BELT_TOP + 28), 180, "BAY 4", 4.0, folder=F)

    # Decorative E-stop on the left leg (not interactive in this training).
    box("EStop_Box", V(FRONT_X - 6, CONV_Y - CONV_LEN / 2 + 8, 80), (10, 10, 12), MI_YELLOW, folder=F)
    cyl("EStop_Mushroom", V(FRONT_X - 12, CONV_Y - CONV_LEN / 2 + 8, 80), 7, 4, MI_RED, rot=R(pitch=90), folder=F)

    line_flow = flow("Flow_Line2", V(CONV_X, HY1 - 6, BELT_TOP), V(CONV_X, CONV_Y - CONV_LEN / 2 - 25, BELT_TOP), 28.0, 62.0, 7)

    # Painted safety zone around Line 2.
    ZX0, ZX1, ZY0, ZY1, SW = 58.0, 235.0, -200.0, HY1 + 20, 6.0
    M = "Environment/Markings"
    box("Zone2_Front", V((ZX0 + ZX1) / 2, ZY0, 0.3), (ZX1 - ZX0, SW, 0.6), MI_STRIPE, collide=False, folder=M)
    box("Zone2_Back", V((ZX0 + ZX1) / 2, ZY1, 0.3), (ZX1 - ZX0, SW, 0.6), MI_STRIPE, collide=False, folder=M)
    box("Zone2_Left", V(ZX0, (ZY0 + ZY1) / 2, 0.3), (SW, ZY1 - ZY0, 0.6), MI_STRIPE, collide=False, folder=M)
    box("Zone2_Right", V(ZX1, (ZY0 + ZY1) / 2, 0.3), (SW, ZY1 - ZY0, 0.6), MI_STRIPE, collide=False, folder=M)
    # Green "stand here" pad at the start spot.
    box("StandPad", V(0, 0, 0.25), (90, 90, 0.5), MI_GREEN, collide=False, folder=M)
    box("Walkway_L", V(cx, -260, 0.3), (X1 - X0 - 40, 8, 0.6), MI_STRIPE, collide=False, folder=M)
    box("Walkway_R", V(cx, 260, 0.3), (X1 - X0 - 40, 8, 0.6), MI_STRIPE, collide=False, folder=M)

    # ------------------------------------------------------------ Line 1 (background, always running)
    L1X = 470.0
    for i, y in enumerate((-200.0, 0.0, 200.0)):
        mesh(f"Line1_Conveyor_{i}", CONVEYOR, V(L1X, y, STAND_H), V(1, 1, 1), MI_BELT, rot=R(yaw=90), folder="Line1")
        for j, dy in enumerate((-92.0, 92.0)):
            box(f"Line1_Leg_{i}_{j}", V(L1X, y + dy, STAND_H / 2), (CONV_W - 10, 8, STAND_H), MI_STEEL, folder="Line1")
    for i, y in enumerate((-305.0, 305.0)):
        box(f"Line1_Hood_{i}", V(L1X, y, BELT_TOP + 30), (CONV_W + 14, 30, 64), MI_YELLOW, folder="Line1")
    flow("Flow_Line1", V(L1X, -300, BELT_TOP), V(L1X, 300, BELT_TOP), 40.0, 70.0, 21)
    box("Zone1_Front", V(L1X - 65, 0, 0.3), (6, 640, 0.6), MI_STRIPE, collide=False, folder=M)
    text("Sign_Line1", V(X1 - 2, 0, 250), 180, "LINE 1", 16.0, color=(0.9, 0.9, 0.85))

    # ------------------------------------------------------------ racking (left wall), pallets, safety kit
    RY_F, RY_B = -470.0, -570.0
    for i, x in enumerate((60.0, 200.0, 340.0, 480.0)):
        for j, y in enumerate((RY_F, RY_B)):
            box(f"Rack_Upright_{i}_{j}", V(x, y, 160), (8, 8, 320), MI_RACK_ORANGE, folder="Environment/Racking")
    for k, z in enumerate((110.0, 220.0, 315.0)):
        for j, y in enumerate((RY_F, RY_B)):
            box(f"Rack_Beam_{k}_{j}", V(270, y, z), (430, 6, 10), MI_RACK_BLUE, folder="Environment/Racking")
    for i, x in enumerate((130.0, 270.0, 410.0)):
        box_stack(f"Rack_Floor_{i}", x, -520, 0, 2 if i != 1 else 1, folder="Environment/Racking")
        box(f"Rack_L1_{i}", V(x, -520, 115 + 7), (120, 100, 14), MI_PALLET, folder="Environment/Racking")
        box(f"Rack_L1_{i}_Boxes", V(x, -520, 129 + 30), (110, 90, 60 - i * 8), MI_CARDBOARD if i % 2 else MI_CARDBOARD_DARK,
            folder="Environment/Racking")
    box_stack("Pallet_A", 240, 430, 8, 3)
    box_stack("Pallet_B", 400, 470, -5, 2)
    box_stack("Pallet_C", -140, 460, 3, 2)

    box("Cabinet_MCC", V(-200, -560, 100), (60, 50, 200), MI_CABINET, folder="Environment/Props")
    text("Sign_MCC", V(-200, -534, 170), 90, "MCC-2\n415V", 7.0, color=(0.05, 0.05, 0.05))
    cyl("FireExt", V(-230, 580, 45), 16, 55, MI_RED, folder="Environment/Props")
    box("FireExt_Sign", V(-230, 598, 115), (30, 1, 20), MI_RED, folder="Environment/Props")
    text("FireExt_Text", V(-230, 596, 115), -90, "FIRE", 7.0, color=(1, 1, 1))

    # ------------------------------------------------------------ control box (right, within reach)
    S = V(37.0, 69.0, 0.0)
    d = unreal.Vector(-S.x, -S.y, 0).normal()
    yaw_d = math.degrees(math.atan2(d.y, d.x))
    TILT, TOP_Z, TOP_T = 32.0, 102.0, 8.0
    p_ = math.radians(90 - TILT)
    n = d * math.cos(p_) + V(0, 0, math.sin(p_))
    u = d * -math.sin(p_) + V(0, 0, math.cos(p_))
    C = V(S.x, S.y, TOP_Z) - d * 4.0
    top_rot = R(pitch=90 - TILT, yaw=yaw_d)
    CF = "ControlBox"
    box("ControlBox_Top", C, (TOP_T, 82, 52), MI_CABINET, rot=top_rot, folder=CF)
    box("ControlBox_Post", V(S.x, S.y, 46), (16, 16, 92), MI_STEEL, rot=R(yaw=yaw_d), folder=CF)
    box("ControlBox_Base", V(S.x, S.y, 2), (44, 44, 4), MI_STEEL, rot=R(yaw=yaw_d), folder=CF)
    text("ControlBox_Title", C + n * (TOP_T / 2 + 0.3) + u * 22.5, yaw_d, "LINE 2 CONTROL", 2.6,
         color=(0.05, 0.05, 0.05), folder=CF, pitch=90 - TILT)

    def button_at(label, words, s, size, text_size, cap, toggle=False):
        loc = C + n * (TOP_T / 2 + 3.25) + u * s
        b = spawn(unreal.TrainerButton, loc, top_rot, label)
        b.set_editor_property("label", words)
        b.set_editor_property("size", unreal.Vector2D(size[0], size[1]))
        b.set_editor_property("text_size", text_size)
        b.set_editor_property("is_toggle", toggle)
        b.set_editor_property("cap_color", unreal.LinearColor(cap[0], cap[1], cap[2], 1))
        b.set_folder_path(CF)
        return b

    GREEN, YELLOW, RED = (0.03, 0.45, 0.08), (0.95, 0.62, 0.0), (0.6, 0.03, 0.02)
    action = button_at("Button_Action", "Start", 12.5, (40, 10), 2.8, GREEN)
    choices = [button_at(f"Button_Choice_{i}", f"Choice {i}", 1.0 - i * 11.0, (74, 9), 2.4, YELLOW) for i in range(3)]

    # ------------------------------------------------------------ boards
    W = unreal.TrainerPanelStyle
    instr = panel("Panel_Instructions", V(190, 40, 185), 130, 84, "Conveyor inspection", "Instructions appear here.", "",
                  W.WHITEBOARD, body_size=4.4, title_size=6.0, footer_size=2.9)
    board_legs("Panel_Instructions", instr, 130, 185 - 42)

    clip_loc = V(10, -80, 125)
    work = panel("Panel_WorkOrder", clip_loc, 52, 70, "Work order #CT-0142", "Work order details.", "",
                 W.CLIPBOARD, body_size=2.3, title_size=3.0, footer_size=1.7, pitch=28)
    cy_ = face_player(clip_loc.x, clip_loc.y)
    back = unreal.MathLibrary.get_forward_vector(R(yaw=cy_)) * -8.0
    box("Clipboard_Post", V(clip_loc.x + back.x, clip_loc.y + back.y, 54), (5, 5, 108), MI_STEEL, rot=R(yaw=cy_), folder="UI")
    box("Clipboard_Base", V(clip_loc.x + back.x, clip_loc.y + back.y, 1.5), (34, 34, 3), MI_STEEL, rot=R(yaw=cy_), folder="UI")

    sign = panel("Panel_SimulationSign", V(X1 - 1, -260, 300), 220, 44, "Training simulation",
                 "Familiarisation only. Not a real lockout/tagout procedure or safety certification.", "",
                 W.SIGN, body_size=5.0, title_size=8.0, yaw=180)
    ppe = panel("Panel_PPE", V(150, Y1 - 1, 210), 120, 40, "PPE required", "Safety glasses, gloves and safety boots in this bay.", "",
                W.SIGN, body_size=4.2, title_size=7.0, yaw=-90)

    # ------------------------------------------------------------ logic
    interactor = spawn(unreal.XRHandInteractor, V(0, 0, 0), None, "XRHandInteractor")
    interactor.set_folder_path("Logic")
    mgr = spawn(unreal.TrainerStepManager, V(0, 0, 250), None, "TrainerStepManager")
    mgr.set_editor_property("instruction_panel", instr)
    mgr.set_editor_property("work_order_panel", work)
    mgr.set_editor_property("action_button", action)
    mgr.set_editor_property("choice_buttons", choices)
    mgr.set_editor_property("parts", [belt, roller, motor])
    mgr.set_editor_property("interactor", interactor)
    mgr.set_editor_property("line_flow", line_flow)
    mgr.set_folder_path("Logic")

    # ------------------------------------------------------------ playtest extras
    if playtest:
        pt_loc = V(-30, -168, 165)
        pt = panel("Panel_Playtest", pt_loc, 86, 58, "Playtest build", "Stats appear here.", "",
                   W.WHITEBOARD, body_size=3.0, title_size=4.2, footer_size=2.1, folder="Playtest")
        board_legs("Panel_Playtest", pt, 86, 165 - 29, folder="Playtest")
        pyaw = face_player(-25, -130)
        reset = spawn(unreal.TrainerButton, V(-25, -130, 100), R(pitch=90 - 35, yaw=pyaw), "Button_ResetRun")
        reset.set_editor_property("label", "Reset run")
        reset.set_editor_property("size", unreal.Vector2D(34, 10))
        reset.set_editor_property("text_size", 2.6)
        reset.set_editor_property("cap_color", unreal.LinearColor(RED[0], RED[1], RED[2], 1))
        reset.set_folder_path("Playtest")
        box("ResetPost", V(-25 - 6 * math.cos(math.radians(pyaw)), -130 - 6 * math.sin(math.radians(pyaw)), 48),
            (10, 10, 96), MI_STEEL, rot=R(yaw=pyaw), folder="Playtest")
        mgr.set_editor_property("playtest_mode", True)
        mgr.set_editor_property("playtest_panel", pt)
        mgr.set_editor_property("reset_button", reset)
        text("Floor_Playtest", V(-60, -60, 0.8), 0, "PLAYTEST", 10.0, color=(0.75, 0.52, 0.0), folder="Playtest", pitch=90)

    # ------------------------------------------------------------ player, nav, lights
    spawn(unreal.PlayerStart, V(0, 0, 120), R(yaw=0), "PlayerStart")

    nav = spawn(unreal.NavMeshBoundsVolume, V(cx, cy, 20), None, "NavMeshBounds")
    o, e = nav.get_actor_bounds(False)
    if e.x > 1:
        nav.set_actor_scale3d(V((X1 - X0 - 40) / (2 * e.x), (Y1 - Y0 - 40) / (2 * e.y), 60 / (2 * max(e.z, 1))))

    hanging_lamp("Light_Line2", 140, 30, 330, H - 10, 24.0, 900.0, True)
    hanging_lamp("Light_Line1", 470, 0, 380, H - 10, 18.0, 900.0, False)
    hanging_lamp("Light_Racking", 270, -400, 380, H - 10, 14.0, 800.0, False)
    point_light("Light_Fill_Player", V(-120, 0, 300), 8.0, 900.0, False)

    # Fixed exposure so dark ink stays dark and the look does not drift with where you look.
    ppv = spawn(unreal.PostProcessVolume, V(0, 0, 200), None, "PostProcess_FixedExposure")
    ppv.set_editor_property("unbound", True)
    pp = ppv.get_editor_property("settings")
    pp.set_editor_property("override_auto_exposure_method", True)
    pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    pp.set_editor_property("override_auto_exposure_bias", True)
    pp.set_editor_property("auto_exposure_bias", EXPOSURE_BIAS)
    pp.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    pp.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
    pp.set_editor_property("override_bloom_intensity", True)
    pp.set_editor_property("bloom_intensity", 0.1)
    ppv.set_editor_property("settings", pp)
    ppv.set_folder_path("Lighting")

    unreal.EditorLoadingAndSavingUtils.save_map(world, level_path)
    log("saved", level_path, "actors:", len(actors.get_all_level_actors()))


build(MAPS + "/L_ConveyorTrainer", playtest=False)
build(MAPS + "/L_ConveyorTrainer_Playtest", playtest=True)
unreal.EditorLoadingAndSavingUtils.load_map(MAPS + "/L_ConveyorTrainer")

with open(unreal.Paths.project_saved_dir() + "build_level.txt", "w") as f:
    f.write("\n".join(LOG))
