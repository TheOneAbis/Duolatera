import unreal

#returns :
#1. the asset_path with SM_prefix added by Unreal IMergeActor tool
#2. the pivot point of the first static mesh 
def select_pcg_actors():
    # Get the editor subsystems
    editor_asset_lib = unreal.EditorAssetLibrary()
    editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    
    all_actors = editor_actor_subsystem.get_all_level_actors()
    #Cleanning up, make sure there's no empty static mesh actor
    print("Iterating all Static Mesh Actor, deleting any empty one")
    for actor in all_actors:
        components = actor.get_components_by_class(unreal.StaticMeshComponent)
        if len(components) > 0:
            empty = False
            for comp in components:
                sm = comp.get_editor_property("static_mesh")
                if sm == None:
                    empty = True
                    continue
                path = comp.get_editor_property("static_mesh").get_path_name()

            if empty == True:
                unreal.log_warning(f"{actor.get_name()} is an empty static mesh actor, deleting")
                editor_actor_subsystem.destroy_actor(actor)

    print("Selecting all actors with PCGComponents")

    pcg_actors = []
    for actor in all_actors:
        components = actor.get_components_by_class(unreal.PCGComponent)
        if len(components) > 0:
            pcg_actors.append(actor)
    
    if len(pcg_actors) == 0:
        unreal.log_warning("No PCG actors found in current level, end script")
        return '', unreal.Vector(0,0,0)
    
    # Create the output directory if it doesn't exist
    output_path = f"/Game/Assets/PCG/MergedSM/{editor_subsystem.get_editor_world().get_name()}"
    
    if not editor_asset_lib.does_directory_exist(output_path):
        editor_asset_lib.make_directory(output_path)
        print(f"path not found, created the folder at {output_path}")
    else:
        print("Output path already exists, new static mesh assets will be stored here")

    #prepare results
    count = 0
    asset_name = f"MERGED_Static_Mesh_{count}"
    asset_path = f"{output_path}/SM_{asset_name}"
    while(editor_asset_lib.does_asset_exist(asset_path)):
        count += 1
        asset_name = f"MERGED_Static_Mesh_{count}"
        asset_path = f"{output_path}/SM_{asset_name}"
    

    #select all PCG actors for the merging
    editor_actor_subsystem.set_selected_level_actors(pcg_actors)
    pivot_location = pcg_actors[0].get_actor_location()
    print(f"Output path: {output_path}")
    print(f"File Name (Copy this): {asset_name}")
    return asset_path, pivot_location


#execute
with unreal.ScopedEditorTransaction("Merge PCG To Static Mesh Transaction") as trans:
    asset_path, pivot_location = select_pcg_actors()