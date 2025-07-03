import unreal
from pathlib import Path
import random


# Custom importer
    
#     Scope: Static Mesh and Skeletal Mesh, with their textures.
#     After validating and importing a .fbx file, create custom material instance, assigning textures

#     1. Check if asset already exists in content browser, skip if it does
#     2. Import every FBX file with its associate texture files (found by name)
#     3. If FBX file is static mesh, check if it meets triangle count limit, if it exceeds the limit, delete and log warning
#     4. If FBX file is static mesh, apply default LOD group to it
#     5. Create a material instnace inherited from M_Cel
#     6. Assign all associate textures to the material instance, setting other necessary parameters as well
#     7. Apply the newly created material instance to the mesh (both static and skeletal mesh)
#     8. Check out (Mark for add) imported files

#     TODO:
#     - Create physics asset for skeletal mesh after importing
#     - Review source control, now is simply unreal.EditorAssetLibrary.save_loaded_assets(), may need further validation
#     - Make the whole process async (can be canceled)

#     Temporal script running solution: 
#     Run the line below in cmd (Python)
#     D:\Document\T2\Content\Utilities\CustomAssetImporter.py


    
def CustomImport(assetRootPath : str):

    # Retrive current mesh assets in content browser
    attemptedAssetCount = 0
    ARH = unreal.AssetRegistryHelpers.get_asset_registry()
    gameMeshesPath = r"/Game/Assets/Models"
    EAL = unreal.EditorAssetLibrary()
    if not EAL.does_directory_exist(gameMeshesPath):
        unreal.log_error("Invalid content browser directory")
        return
    existingAssets = ARH.get_assets_by_path(gameMeshesPath)
    existingAssetNames : list[str] = []
    for asset in existingAssets:
        #print(asset.asset_name)
        existingAssetNames.append(str(asset.asset_name))
        attemptedAssetCount += 1
    print(f"{attemptedAssetCount} assets found in project: {gameMeshesPath}")
    

    # Iterate through asset file folder outside of project
    print(f"Start iterating through external file directory: {assetRootPath}")
    if not (unreal.Paths.directory_exists(assetRootPath)):
        unreal.log_error("Invalid asset root path")
        return
    directoryPath = Path(assetRootPath)
    
    # prepare the parent cel shading material
    materialParent = EAL.load_asset("/Game/Assets/Materials/M_Cel")

    attemptedAssetCount = 0
    deletedassetCount = 0
    for filePath in directoryPath.iterdir():
        fileName = str(filePath).split('\\')[-1]
        if (filePath.is_file() and  
            fileName.split('.')[1].lower() == 'fbx' and # check suffix
            (fileName.split('_')[0].lower() == 'skm' or fileName.split('_')[0].lower() =='sm')): #check prefix
            
            assetName = fileName.split('.')[0]
            
            if(assetName not in existingAssetNames):
                print(f"------------- Importing {assetName} -------------") 
                attemptedAssetCount += 1
                
                # find all associated assets (textures)
                importFilePaths = FindAssociateFiles(directoryPath, assetName.split("_")[1])
                importFilePaths.append(str(filePath))

                print("Importing files:")
                for path in importFilePaths:
                    print(path)

                destinationDir = r"/Game/Assets/Models/AutomationImport"
                importedAssets = ImportAssociatedAssets(destinationDir, importFilePaths)

                for asset in importedAssets: print(f"imported asset: {asset}")
                
                staticMeshassetPath = destinationDir + r"/" + assetName
                loadedAsset = unreal.load_asset(staticMeshassetPath)
                
                if isinstance(loadedAsset, unreal.StaticMesh):
                    # After importing, first, check if the static mesh triangle count is within limit
                    # if trianlge count not within limit, delete the mesh and its associated assets
                    triangleCountLimit = 10000
                    if loadedAsset.get_num_triangles(0) > triangleCountLimit:
                        unreal.log_warning(f"{str(filePath)}: Triangle count exceed limit, deleting mesh and its associated assets")
                        DeleteAssociatedAssets(importedAssets)
                        deletedassetCount += 1
                        continue

                    # Set default LOD 
                    if loadedAsset.get_num_lods() <= 1:
                         print("Setting LOD Group to 'Deco'")
                         loadedAsset.set_editor_property("lod_group", "Deco")

                # Create the material instance inherited from Cel Shader 
                assetNameStripped = assetName.split('_')[-1]
                if EAL.does_asset_exist(destinationDir + r"/" + f"MI_Cel_{assetNameStripped}"):
                    print(f"Delete prexisitng material instance: " + destinationDir + r"/" + f"MI_Cel_{assetNameStripped}")
                    EAL.delete_asset(destinationDir + r"/" + f"MI_Cel_{assetNameStripped}")
                
                ATH = unreal.AssetToolsHelpers.get_asset_tools()
                materialInstance = ATH.create_asset(f"MI_Cel_{assetNameStripped}", 
                                                    destinationDir, 
                                                    unreal.MaterialInstanceConstant, 
                                                    unreal.MaterialInstanceConstantFactoryNew())
                
                # assign parent material and textures
                materialInstance.set_editor_property('parent', materialParent)
                if len(importedAssets) <= 1:
                    # No associated textures found with this fbx, skipping
                    defaultColor = unreal.LinearColor(1,0,1,1)
                    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(materialInstance, "Color Texture", defaultColor)
                else:
                    for assetObj in importedAssets:
                        if not isinstance(assetObj, unreal.Texture):
                            continue
                        if not assetObj.get_name().split('_')[0] == "T":
                            unreal.log_error(f"Invalid texture asset naming: {assetObj.get_name()}")
                            continue

                        # Mask or Normal Texture
                        if assetObj.get_name().split('_')[2] == "Mask":
                            index = assetObj.get_name().split('_')[3]
                            unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(materialInstance, f"RGBTexture{index}", assetObj)
                            if int(index) > 1:
                                unreal.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(materialInstance, f"UseRGBTexture{index}?", True)
                            
                            RGBColors : list[unreal.LinearColor] = []
                            for i in range(3): 
                                RGBColors.append(unreal.LinearColor(random.uniform(0.0, 1.0),
                                                                    random.uniform(0.0, 1.0),
                                                                    random.uniform(0.0, 1.0),
                                                                    1))
                            unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(materialInstance, f"R Channel Color {index}", RGBColors[0])
                            unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(materialInstance, f"G Channel Color {index}", RGBColors[1])
                            unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(materialInstance, f"B Channel Color {index}", RGBColors[2])
                        
                        elif assetObj.get_name().split('_')[2] == "Normal":
                            unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(materialInstance, "Normal Texture", assetObj)
                            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(materialInstance, "Normal Intensity", 1.0)
                
                # After setting up the material instance, apply it to the static / skeletal mesh
                if isinstance(loadedAsset, unreal.StaticMesh):
                    loadedAsset.set_material(0, materialInstance)
                elif isinstance(loadedAsset, unreal.SkeletalMesh):
                    skeletalMaterial = unreal.SkeletalMaterial()
                    skeletalMaterial.material_interface = materialInstance
                    skeletalMaterial.material_slot_name = materialInstance.get_name()

                    materialArray = unreal.Array(unreal.SkeletalMaterial)
                    materialArray.append(skeletalMaterial)
                    loadedAsset.materials = materialArray
                
    # Finally, check out files in source control
    print("checkout pass")
    assets = EAL.list_assets(destinationDir)
    for asset in assets:
        print(asset)
        EAL.save_asset(asset)
    

                    
                          
    print(f"{attemptedAssetCount} newly found asset groups and attempted importing, {deletedassetCount} asset groups deleted for not meeting criteria")
    

def DeleteAssociatedAssets(importedAssets):
    EAL = unreal.EditorAssetLibrary()
    for asset in importedAssets:
        EAL.delete_asset(asset.get_package().get_name())
    

def ImportAssociatedAssets(destinationDir : str, importFilePaths : list[str]) -> unreal.Array[unreal.Object]:
    EAL = unreal.EditorAssetLibrary()
    if not EAL.does_directory_exist(destinationDir):
        unreal.log_error("Invalid content browser directory")
        return None
    
    # Start importing
    assetTools = unreal.AssetToolsHelpers.get_asset_tools()
    assetImportData = unreal.AutomatedAssetImportData()
    assetImportData.destination_path = destinationDir
    assetImportData.filenames = importFilePaths
    assetImportData.replace_existing = True
    
    return assetTools.import_assets_automated(assetImportData)

def FindAssociateFiles(directoryPath, assetNameStripped : str) -> list[str]:
    # return the full path of the file to be imported
    associateFilePaths : list[str] = []
    
    for filePath in directoryPath.iterdir():
        if not filePath.is_file(): continue
            
        fileName = str(filePath).split('\\')[-1]
        try:
            fileNameStripped = fileName.split('_')[1]
        except:
            unreal.log_error(f"Invalid file naming convention: {str(filePath)}")
            continue

        if(fileNameStripped == assetNameStripped):
            #print(f"associate file: {fileName}" )
            associateFilePaths.append(str(filePath))
    return associateFilePaths

# D:\Document\Github Projects\Duolatera-art\3D Aseets
# D:\Document\Desktop\AutomationImportTest
assetRootPathString = r"D:\Document\Desktop\AutomationImportTest"
CustomImport(assetRootPathString)