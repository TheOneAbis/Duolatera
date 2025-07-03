import unreal

def spawn_static_mesh(asset_path, pivot_location):
    editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    #After merging, Unreal merge tool automatically adds SM_ prefix to the file name
    # Load the object
    loaded_object = unreal.load_asset(asset_path)
    if loaded_object == None:
        unreal.log_warning("No asset path stored, did you click the first button first?")
        return

    actor_rotation = unreal.Rotator(0,0,0)
    spawned_actor = editor_actor_subsystem.spawn_actor_from_object(loaded_object, pivot_location, actor_rotation)

    # Get all pcg actors in the current level
    all_actors = editor_actor_subsystem.get_all_level_actors()

    for actor in all_actors:
        components = actor.get_components_by_class(unreal.PCGComponent)
        if len(components) > 0:
            editor_actor_subsystem.destroy_actor(actor)
    

#execute
with unreal.ScopedEditorTransaction("Merge PCG To Static Mesh Transaction") as trans:
    spawn_static_mesh(asset_path, pivot_location)