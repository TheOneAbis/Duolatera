import unreal
from pathlib import Path
import random

def FindAssociateFiles(externalAssetDir, assetPath : str) -> unreal.Array[str]:
    # return the full path of the file to be imported
    associateFilePaths : unreal.Array[str] = []
    
    try:
        assetName = str(assetPath).split('\\')[-1].split('.')[0]
        # print(f"Test asset name: {assetName}")
    except:
        unreal.log_error(f"Can't extract pure file name from {assetPath}")
        return None
    
    associateFilePaths.append(assetPath)

    directoryPath = Path(externalAssetDir)
    for filePath in directoryPath.iterdir():
        if not filePath.is_file(): continue
            
        fileName = str(filePath).split('\\')[-1]
        try:
            fileNameStripped = fileName.split('_')[1]
        except:
            unreal.log_error(f"Invalid file naming convention: {str(filePath)}")
            continue

        if fileNameStripped in assetName:
            #print(f"associate file: {fileName}" )
            associateFilePaths.append(str(filePath))
    
    return associateFilePaths

def ImportAssets(destinationDir : str, importFilePaths : unreal.Array[str]) -> unreal.Array[unreal.Object]:
    EAL = unreal.EditorAssetLibrary()
    if not EAL.does_directory_exist(destinationDir):
        unreal.log_error("Invalid content browser directory")
        return None
        
    # Start importing using AutomatedAssetImportData
    assetTools = unreal.AssetToolsHelpers.get_asset_tools()
    assetImportData = unreal.AutomatedAssetImportData()
    assetImportData.destination_path = destinationDir
    assetImportData.filenames = importFilePaths
    assetImportData.replace_existing = True
    
    importedAssets = assetTools.import_assets_automated(assetImportData)
    
    return importedAssets

def RemoveDuplicateAssets(importedAssets : unreal.Array[unreal.Object]) -> unreal.Array[unreal.Object]:
    cleanImportedAssets : unreal.Array[unreal.Object] = []
    for asset in importedAssets:
        if asset not in cleanImportedAssets:
            cleanImportedAssets.append(asset)
    
    return cleanImportedAssets

#import code starts below
def ValidateStaticMesh(destinationDir : str, assetPath: str, importedAssets) -> {str, int}:
    try:
        assetName = str(assetPath).split('\\')[-1].split('.')[0]
    except:
        errorInfo = f"Invalid naming convention: {assetPath}"
        unreal.log_error(errorInfo)
        DeleteAssociatedAssets(importedAssets)
        return errorInfo, 1
    
    staticMeshassetPath = destinationDir + r"/" + assetName
    loadedAsset = unreal.load_asset(staticMeshassetPath)

    if isinstance(loadedAsset, unreal.StaticMesh):
        # After importing, first, check if the static mesh triangle count is within limit
        # if trianlge count not within limit, delete the mesh and its associated assets
        triangleCountLimit = 10000
        if loadedAsset.get_num_triangles(0) > triangleCountLimit:
            errorInfo = f"{str(assetName)}: Triangle count exceed limit, deleting mesh and its associated assets"
            unreal.log_warning(errorInfo)
            DeleteAssociatedAssets(importedAssets)
            return errorInfo, 1

        # Set default LOD 
        if loadedAsset.get_num_lods() <= 1:
             print("Setting LOD Group to 'Deco'")
             loadedAsset.set_editor_property("lod_group", "Deco")
    
    importInfo = f"{str(assetName)} triangle count within limit, LOD group set to 'Deco'. "
    return importInfo, 5

def DeleteAssociatedAssets(importedAssets):
    EAL = unreal.EditorAssetLibrary()
    for asset in importedAssets:
        EAL.delete_asset(asset.get_package().get_name())
              
def GenerateNamingConventionErrorMessage(assetName) -> str:
    return f"Invalid naming convention for {assetName}, please check source file in external asset direcoty. \n"

def CreateMaterialInstanceForFBX(importedAssets : unreal.Array[unreal.Object], destinationDir : str, materialParentPath : str) -> {unreal.Array[unreal.Object], str, int}:    
    # Check for static mesh or skeletal mesh exists
    fbxAsset : unreal.Object = None
    for asset in importedAssets:
        if isinstance(asset, unreal.StaticMesh) or isinstance(asset, unreal.SkeletalMesh):
            fbxAsset = asset
            break
    if fbxAsset == None:
        unreal.log_error("Can't find imported fbx asset when creating the material for it")
        noFbxError = "No StaticMesh or SkeletalMesh found in imported files, please check external asset directory. \n"
        return importedAssets, noFbxError, 0
    
    try:
        assetNameStripped = fbxAsset.get_name() #SM_BridgePillarTop
        assetNameStripped = assetNameStripped.split('_')[-1]
    except:
        namingConventionError = GenerateNamingConventionErrorMessage(fbxAsset.get_name())
        return importedAssets, namingConventionError, 4
    
    # Checks done, now create the material instance
    returnAssets = importedAssets
    ATH = unreal.AssetToolsHelpers.get_asset_tools()
    EAL = unreal.EditorAssetLibrary()
    returnInfo : str = ""

    # if material instance with the same name exists, delete old file
    if EAL.does_asset_exist(destinationDir + r"/" + f"MI_Cel_{assetNameStripped}"):
        unreal.log_warning(f"Delete prexisitng material instance: " + destinationDir + r"/" + f"MI_Cel_{assetNameStripped}")
        EAL.delete_asset(destinationDir + r"/" + f"MI_Cel_{assetNameStripped}")
        returnInfo += destinationDir + r"/" + f"MI_Cel_{assetNameStripped}" + " had existed in destination folder, now overwritten by the new material instance. \n"

    # Create material instance
    materialInstance = ATH.create_asset(f"MI_Cel_{assetNameStripped}", 
                                        destinationDir, 
                                        unreal.MaterialInstanceConstant, 
                                        unreal.MaterialInstanceConstantFactoryNew())
    importedAssets.append(materialInstance)
    
    # prepare the parent cel shading material
    materialParent = EAL.load_asset(materialParentPath)
    # assign parent material and textures
    materialInstance.set_editor_property('parent', materialParent)

    # Check for any texture
    if len(importedAssets) <= 1:
        defaultColor = unreal.LinearColor(1,0,1,1)
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(materialInstance, "Color Texture", defaultColor)
        ApplyMaterialInstanceToFBX(fbxAsset, materialInstance)

        noTextureWarning = "No associated textures found with this fbx file, please check if external asset directory contains its textures. \n"
        unreal.log_warning(noTextureWarning)
        returnInfo += noTextureWarning
        returnAssets.append(materialInstance)
        return returnAssets, noTextureWarning, 3

    # If the batch has textures:
    for asset in importedAssets:
        if not isinstance(asset, unreal.Texture):
            continue
        if not asset.get_name().split('_')[0] == "T":
            namingConventionError = GenerateNamingConventionErrorMessage(asset.get_name())
            returnInfo += namingConventionError
            unreal.log_error(f"Invalid texture asset naming: {asset.get_name()}")
            continue
        
        #Finally a valid texture asset

        # Mask or Normal Texture
        if asset.get_name().split('_')[2] == "Mask":
            index = asset.get_name().split('_')[3]
            unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(materialInstance, f"RGBTexture{index}", asset)
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
        
        elif asset.get_name().split('_')[2] == "Normal":
            unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(materialInstance, "Normal Texture", asset)
            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(materialInstance, "Normal Intensity", 1.0)

    # After setting up the material instance, apply it to the static / skeletal mesh
    ApplyMaterialInstanceToFBX(fbxAsset, materialInstance)

    return importedAssets, returnInfo, 5

def ApplyMaterialInstanceToFBX(fbxAsset, materialInstance):
    if isinstance(fbxAsset, unreal.StaticMesh):
        fbxAsset.set_material(0, materialInstance)
    elif isinstance(fbxAsset, unreal.SkeletalMesh):
        skeletalMaterial = unreal.SkeletalMaterial()
        skeletalMaterial.material_interface = materialInstance
        skeletalMaterial.material_slot_name = materialInstance.get_name()
        
        materialArray = unreal.Array(unreal.SkeletalMaterial)
        materialArray.append(skeletalMaterial)
        fbxAsset.materials = materialArray  


info : unreal.Array[str] = []
import_status_code : int = 5
tempCode : int = 0

associateFilePaths = FindAssociateFiles(external_asset_dir, asset_path)
imported_assets : unreal.Array[unreal.Object] = ImportAssets(destination_dir, associateFilePaths)

validationInfo, tempCode = ValidateStaticMesh(destination_dir, asset_path, imported_assets)
import_status_code = min(import_status_code, tempCode)

if import_status_code <= 1:
    imported_assets = []
    info.append(validationInfo)
else:
    imported_assets = RemoveDuplicateAssets(imported_assets)
    new_asset_batch, matInfo, tempCode = CreateMaterialInstanceForFBX(imported_assets, destination_dir, material_parent_path)
    import_status_code = min(import_status_code, tempCode)
    info.append(matInfo)
    print(f"Asset {asset_path} with its associate files imported")



#----------------------------------------------------------------

'''
Import Status Code : int
0 import failed
1 import failed (deleted) static mesh failed validation (naming convention, triangle count, etc)
2 import succeeded, but FBX import has warning (caught by blueprint)
3 import succeeded, but no associated exture found
4 import succeeded, but some other problems exist
5 import succeeded without problem

'''






import unreal
from tkinter import *
from tkinter.ttk import *
from tkinter.filedialog import askdirectory

contentPath = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())[:-9] # Remove the chars /Content/ from path
directory = askdirectory(initialdir = unreal.Paths.project_content_dir(), title = "Select Directory", mustexist=True)
directory = directory.replace(contentPath, "").replace("/Content", "/Game")

is_directory_valid = True
is_in_project = True

if directory == "":
    is_directory_valid = False
    is_in_project = False
else:
    is_directory_valid = True
    is_in_project = directory.split('/')[1] == "Game"

#----------------------------------------------------------------


import unreal
from pathlib import Path
def GetNewAssetPaths(externalAssetPath : str, existingAssets : unreal.Array[unreal.AssetData]) -> unreal.Array[str]:
    newAssetFilePaths : unreal.Array[str] = []

    directoryPath = Path(externalAssetPath)
    for filePath in directoryPath.iterdir():
        fileName = str(filePath).split('\\')[-1] # SM_Holster.fbx
        
        if not (filePath.is_file() and  
                fileName.split('.')[1].lower() == 'fbx' and # check suffix
                (fileName.split('_')[0].lower() == 'skm' or fileName.split('_')[0].lower() =='sm')): #check prefix
            continue
            
        isAssetNew : bool = True
        for asset in existingAssets:
            if not isinstance(asset, unreal.AssetData): continue # won't happen, simply casting to unreal.AssetData    
            if fileName.split('.')[0] == asset.asset_name:
                print(f"{filePath} alraedy exists in the project, its package path is: {asset.get_full_name()}")
                isAssetNew = False
                break

        if not isAssetNew: continue

        newAssetFilePaths.append(str(filePath))

    return newAssetFilePaths

new_asset_file_paths = GetNewAssetPaths(external_asset_path, existing_assets)

#----------------------------------------------------------------

is_directory_valid = not(directory == "")


import unreal

def CheckImportDirectoryExist(externalAssetPath : str, projectAssetPath : str):
    # Retrive current mesh assets in content browser
    EAL = unreal.EditorAssetLibrary()
    if not EAL.does_directory_exist(projectAssetPath):
        unreal.log_error("Invalid comparing content browser directory")
        return False
    
    # Iterate through asset file folder outside of project
    print(f"Start iterating through external file directory: {externalAssetPath}")
    if not (unreal.Paths.directory_exists(externalAssetPath)):
        unreal.log_error("Invalid asset root path")
        return False
    
    return True

paths_all_good = CheckImportDirectoryExist(external_asset_path, project_asset_path)

#----------------------------------------------------------------

unreal.AssetImportTask
unreal.AssetImportInfo

import random
def CustomImport(externalAssetPath : str):
    
    directoryPath = Path(externalAssetPath)
    EAL = unreal.EditorAssetLibrary()
    ARH = unreal.AssetRegistryHelpers.get_asset_registry()
    
    attemptedAssetCount = 0
    existingAssets = ARH.get_assets_by_path(projectAssetPath)
    existingAssetNames : list[str] = []
    for asset in existingAssets:
        #print(asset.asset_name)
        existingAssetNames.append(str(asset.asset_name))
        attemptedAssetCount += 1
    print(f"{attemptedAssetCount} assets found in project: {projectAssetPath}")

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
                importedAssets = ImportAssets(destinationDir, importFilePaths)

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
    




# D:\Document\Github Projects\Duolatera-art\3D Aseets
# D:\Document\Desktop\AutomationImportTest
assetRootPathString = r"D:\Document\Desktop\AutomationImportTest"
CustomImport(assetRootPathString)



'''
    UI:
    Been focusing a lot on the UI,  and added constant hints, along with tool tips
    Made a folder select window, for artists to get the desried directory as easy as possible
    Made a tab switching effect, separate different sections with regions

    Code
    Took a good damn time retrieving log messages by diving into code
    Much more error handling, directory validation, wrong naming convention, 

'''

# TODO: log all info back to widget:
# if static mesh passed triangle count validation
# show the imported static/skeletal mesh, texture, material
# print all info
