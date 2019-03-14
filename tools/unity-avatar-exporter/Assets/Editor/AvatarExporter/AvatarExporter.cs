//  AvatarExporter.cs
//
//  Created by David Back on 28 Nov 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

class AvatarExporter : MonoBehaviour {
    // update version number for every PR that changes this file, also set updated version in README file
    static readonly string AVATAR_EXPORTER_VERSION = "0.3.5";
    
    static readonly float HIPS_GROUND_MIN_Y = 0.01f;
    static readonly float HIPS_SPINE_CHEST_MIN_SEPARATION = 0.001f;
    static readonly int MAXIMUM_USER_BONE_COUNT = 256;
    static readonly string EMPTY_WARNING_TEXT = "None";
    static readonly string TEXTURES_DIRECTORY = "textures";
    static readonly string DEFAULT_MATERIAL_NAME = "No Name";
    static readonly string HEIGHT_REFERENCE_PREFAB = "Assets/Editor/AvatarExporter/HeightReference.prefab";
    static readonly Vector3 PREVIEW_CAMERA_PIVOT = new Vector3(0.0f, 1.755f, 0.0f);
    static readonly Vector3 PREVIEW_CAMERA_DIRECTION = new Vector3(0.0f, 0.0f, -1.0f);
    
    // TODO: use regex
    static readonly string[] RECOMMENDED_UNITY_VERSIONS = new string[] {
        "2018.2.12f1",
        "2018.2.11f1",
        "2018.2.10f1",
        "2018.2.9f1",
        "2018.2.8f1",
        "2018.2.7f1",
        "2018.2.6f1",
        "2018.2.5f1",
        "2018.2.4f1",
        "2018.2.3f1",
        "2018.2.2f1",
        "2018.2.1f1",
        "2018.2.0f2",
        "2018.1.9f2",
        "2018.1.8f1",
        "2018.1.7f1",
        "2018.1.6f1",
        "2018.1.5f1",
        "2018.1.4f1",
        "2018.1.3f1",
        "2018.1.2f1",
        "2018.1.1f1",
        "2018.1.0f2",
        "2017.4.18f1",
        "2017.4.17f1",
    };
   
    static readonly Dictionary<string, string> HUMANOID_TO_HIFI_JOINT_NAME = new Dictionary<string, string> {
        {"Chest", "Spine1"},
        {"Head", "Head"},
        {"Hips", "Hips"},
        {"Left Index Distal", "LeftHandIndex3"},
        {"Left Index Intermediate", "LeftHandIndex2"},
        {"Left Index Proximal", "LeftHandIndex1"},
        {"Left Little Distal", "LeftHandPinky3"},
        {"Left Little Intermediate", "LeftHandPinky2"},
        {"Left Little Proximal", "LeftHandPinky1"},
        {"Left Middle Distal", "LeftHandMiddle3"},
        {"Left Middle Intermediate", "LeftHandMiddle2"},
        {"Left Middle Proximal", "LeftHandMiddle1"},
        {"Left Ring Distal", "LeftHandRing3"},
        {"Left Ring Intermediate", "LeftHandRing2"},
        {"Left Ring Proximal", "LeftHandRing1"},
        {"Left Thumb Distal", "LeftHandThumb3"},
        {"Left Thumb Intermediate", "LeftHandThumb2"},
        {"Left Thumb Proximal", "LeftHandThumb1"},
        {"LeftEye", "LeftEye"},
        {"LeftFoot", "LeftFoot"},
        {"LeftHand", "LeftHand"},
        {"LeftLowerArm", "LeftForeArm"},
        {"LeftLowerLeg", "LeftLeg"},
        {"LeftShoulder", "LeftShoulder"},
        {"LeftToes", "LeftToeBase"},
        {"LeftUpperArm", "LeftArm"},
        {"LeftUpperLeg", "LeftUpLeg"},
        {"Neck", "Neck"},
        {"Right Index Distal", "RightHandIndex3"},
        {"Right Index Intermediate", "RightHandIndex2"},
        {"Right Index Proximal", "RightHandIndex1"},
        {"Right Little Distal", "RightHandPinky3"},
        {"Right Little Intermediate", "RightHandPinky2"},
        {"Right Little Proximal", "RightHandPinky1"},
        {"Right Middle Distal", "RightHandMiddle3"},
        {"Right Middle Intermediate", "RightHandMiddle2"},
        {"Right Middle Proximal", "RightHandMiddle1"},
        {"Right Ring Distal", "RightHandRing3"},
        {"Right Ring Intermediate", "RightHandRing2"},
        {"Right Ring Proximal", "RightHandRing1"},
        {"Right Thumb Distal", "RightHandThumb3"},
        {"Right Thumb Intermediate", "RightHandThumb2"},
        {"Right Thumb Proximal", "RightHandThumb1"},
        {"RightEye", "RightEye"},
        {"RightFoot", "RightFoot"},
        {"RightHand", "RightHand"},
        {"RightLowerArm", "RightForeArm"},
        {"RightLowerLeg", "RightLeg"},
        {"RightShoulder", "RightShoulder"},
        {"RightToes", "RightToeBase"},
        {"RightUpperArm", "RightArm"},
        {"RightUpperLeg", "RightUpLeg"},
        {"Spine", "Spine"},
        {"UpperChest", "Spine2"},
    };
    
    // absolute reference rotations for each Humanoid bone using Artemis fbx in Unity 2018.2.12f1
    static readonly Dictionary<string, Quaternion> REFERENCE_ROTATIONS = new Dictionary<string, Quaternion> {
        {"Chest", new Quaternion(-0.0824653f, 1.25274e-7f, -6.75759e-6f, 0.996594f)},
        {"Head", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"Hips", new Quaternion(-3.043941e-10f, -1.573706e-7f, 5.112975e-6f, 1f)},
        {"Left Index Distal", new Quaternion(-0.5086057f, 0.4908088f, -0.4912299f, -0.5090388f)},
        {"Left Index Intermediate", new Quaternion(-0.4934928f, 0.5062312f, -0.5064303f, -0.4936835f)},
        {"Left Index Proximal", new Quaternion(-0.4986293f, 0.5017503f, -0.5013659f, -0.4982448f)},
        {"Left Little Distal", new Quaternion(-0.490056f, 0.5143053f, -0.5095307f, -0.4855038f)},
        {"Left Little Intermediate", new Quaternion(-0.5083722f, 0.4954255f, -0.4915887f, -0.5044324f)},
        {"Left Little Proximal", new Quaternion(-0.5062528f, 0.497324f, -0.4937346f, -0.5025966f)},
        {"Left Middle Distal", new Quaternion(-0.4871885f, 0.5123404f, -0.5125002f, -0.4873383f)},
        {"Left Middle Intermediate", new Quaternion(-0.5171652f, 0.4827828f, -0.4822642f, -0.5166069f)},
        {"Left Middle Proximal", new Quaternion(-0.4955998f, 0.5041052f, -0.5043675f, -0.4958555f)},
        {"Left Ring Distal", new Quaternion(-0.4936301f, 0.5097645f, -0.5061787f, -0.4901562f)},
        {"Left Ring Intermediate", new Quaternion(-0.5089865f, 0.4943658f, -0.4909532f, -0.5054707f)},
        {"Left Ring Proximal", new Quaternion(-0.5020972f, 0.5005084f, -0.4979034f, -0.4994819f)},
        {"Left Thumb Distal", new Quaternion(-0.6617184f, 0.2884935f, -0.3604706f, -0.5907297f)},
        {"Left Thumb Intermediate", new Quaternion(-0.6935627f, 0.1995147f, -0.2805665f, -0.6328092f)},
        {"Left Thumb Proximal", new Quaternion(-0.6663674f, 0.278572f, -0.3507071f, -0.5961183f)},
        {"LeftEye", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"LeftFoot", new Quaternion(0.009215056f, 0.3612514f, 0.9323555f, -0.01121602f)},
        {"LeftHand", new Quaternion(-0.4797408f, 0.5195366f, -0.5279632f, -0.4703038f)},
        {"LeftLowerArm", new Quaternion(-0.4594738f, 0.4594729f, -0.5374805f, -0.5374788f)},
        {"LeftLowerLeg", new Quaternion(-0.0005380471f, -0.03154583f, 0.9994993f, 0.002378627f)},
        {"LeftShoulder", new Quaternion(-0.3840606f, 0.525857f, -0.5957767f, -0.47013f)},
        {"LeftToes", new Quaternion(-0.0002536641f, 0.7113448f, 0.7027079f, -0.01379319f)},
        {"LeftUpperArm", new Quaternion(-0.4591927f, 0.4591916f, -0.5377204f, -0.5377193f)},
        {"LeftUpperLeg", new Quaternion(-0.0006682819f, 0.0006864658f, 0.9999968f, -0.002333928f)},
        {"Neck", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"Right Index Distal", new Quaternion(0.5083892f, 0.4911618f, -0.4914584f, 0.5086939f)},
        {"Right Index Intermediate", new Quaternion(0.4931984f, 0.5065879f, -0.5067145f, 0.4933202f)},
        {"Right Index Proximal", new Quaternion(0.4991491f, 0.5012957f, -0.5008481f, 0.4987026f)},
        {"Right Little Distal", new Quaternion(0.4890696f, 0.5154139f, -0.5104482f, 0.4843578f)},
        {"Right Little Intermediate", new Quaternion(0.5084175f, 0.495413f, -0.4915423f, 0.5044444f)},
        {"Right Little Proximal", new Quaternion(0.5069782f, 0.4965974f, -0.4930001f, 0.5033045f)},
        {"Right Middle Distal", new Quaternion(0.4867662f, 0.5129694f, -0.5128888f, 0.4866894f)},
        {"Right Middle Intermediate", new Quaternion(0.5167004f, 0.4833596f, -0.4827653f, 0.5160643f)},
        {"Right Middle Proximal", new Quaternion(0.4965845f, 0.5031784f, -0.5033959f, 0.4967981f)},
        {"Right Ring Distal", new Quaternion(0.4933217f, 0.5102056f, -0.5064691f, 0.4897075f)},
        {"Right Ring Intermediate", new Quaternion(0.5085972f, 0.494844f, -0.4913519f, 0.505007f)},
        {"Right Ring Proximal", new Quaternion(0.502959f, 0.4996676f, -0.4970418f, 0.5003144f)},
        {"Right Thumb Distal", new Quaternion(0.6611374f, 0.2896575f, -0.3616535f, 0.5900872f)},
        {"Right Thumb Intermediate", new Quaternion(0.6937408f, 0.1986776f, -0.279922f, 0.6331626f)},
        {"Right Thumb Proximal", new Quaternion(0.6664271f, 0.2783172f, -0.3505667f, 0.596253f)},
        {"RightEye", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"RightFoot", new Quaternion(-0.009482829f, 0.3612484f, 0.9323512f, 0.01144584f)},
        {"RightHand", new Quaternion(0.4797273f, 0.5195542f, -0.5279628f, 0.4702987f)},
        {"RightLowerArm", new Quaternion(0.4594217f, 0.4594215f, -0.5375242f, 0.5375237f)},
        {"RightLowerLeg", new Quaternion(0.0005446263f, -0.03177159f, 0.9994922f, -0.002395923f)},
        {"RightShoulder", new Quaternion(0.3841222f, 0.5257177f, -0.5957286f, 0.4702966f)},
        {"RightToes", new Quaternion(0.0001034f, 0.7113398f, 0.7027067f, 0.01411251f)},
        {"RightUpperArm", new Quaternion(0.4591419f, 0.4591423f, -0.537763f, 0.5377624f)},
        {"RightUpperLeg", new Quaternion(0.0006750703f, 0.0008973633f, 0.9999966f, 0.002352045f)},
        {"Spine", new Quaternion(-0.05427956f, 1.508558e-7f, -2.775203e-6f, 0.9985258f)},
        {"UpperChest", new Quaternion(-0.0824653f, 1.25274e-7f, -6.75759e-6f, 0.996594f)},
    };
    
    // Humanoid mapping name suffixes for each set of appendages
    static readonly string[] LEG_MAPPING_SUFFIXES = new string[] {
        "UpperLeg",
        "LowerLeg",
        "Foot",
        "Toes",
    };
    static readonly string[] ARM_MAPPING_SUFFIXES = new string[] {
        "Shoulder",
        "UpperArm",
        "LowerArm",
        "Hand",
    };
    static readonly string[] HAND_MAPPING_SUFFIXES = new string[] {
        " Index Distal",
        " Index Intermediate",
        " Index Proximal",
        " Little Distal",
        " Little Intermediate",
        " Little Proximal",
        " Middle Distal",
        " Middle Intermediate",
        " Middle Proximal",
        " Ring Distal",
        " Ring Intermediate",
        " Ring Proximal",
        " Thumb Distal",
        " Thumb Intermediate",
        " Thumb Proximal",
    };
    
    static readonly string STANDARD_SHADER = "Standard";
    static readonly string STANDARD_ROUGHNESS_SHADER = "Standard (Roughness setup)";
    static readonly string STANDARD_SPECULAR_SHADER = "Standard (Specular setup)";
    static readonly string[] SUPPORTED_SHADERS = new string[] {
        STANDARD_SHADER,
        STANDARD_ROUGHNESS_SHADER,
        STANDARD_SPECULAR_SHADER,
    };

    enum AvatarRule {
        RecommendedUnityVersion,
        SingleRoot,
        NoDuplicateMapping,
        NoAsymmetricalLegMapping,
        NoAsymmetricalArmMapping,
        NoAsymmetricalHandMapping,
        HipsMapped,
        SpineMapped,
        SpineDescendantOfHips,
        ChestMapped,
        ChestDescendantOfSpine,
        NeckMapped,
        HeadMapped,
        HeadDescendantOfChest,
        EyesMapped,
        HipsNotOnGround,
        HipsSpineChestNotCoincident,
        TotalBoneCountUnderLimit,
        AvatarRuleEnd,
    };
    // rules that are treated as errors and prevent exporting, otherwise rules will show as warnings
    static readonly AvatarRule[] EXPORT_BLOCKING_AVATAR_RULES = new AvatarRule[] {
        AvatarRule.HipsMapped,
        AvatarRule.SpineMapped,
        AvatarRule.ChestMapped,
        AvatarRule.HeadMapped,
    };
    
    class UserBoneInformation {
        public string humanName; // bone name in Humanoid if it is mapped, otherwise ""
        public string parentName; // parent user bone name
        public int mappingCount; // number of times this bone is mapped in Humanoid
        public Vector3 position; // absolute position
        public Quaternion rotation; // absolute rotation
        public BoneTreeNode boneTreeNode;
        
        public UserBoneInformation() {
            humanName = "";
            parentName = "";
            mappingCount = 0;
            position = new Vector3();
            rotation = new Quaternion();
            boneTreeNode = new BoneTreeNode();
        }
        
        public bool HasHumanMapping() { return !string.IsNullOrEmpty(humanName); }
    }
    
    class BoneTreeNode { 
        public string boneName;
        public List<BoneTreeNode> children = new List<BoneTreeNode>();
        
        public BoneTreeNode() {}
        public BoneTreeNode(string name) {
            boneName = name;
        }
    }
    
    class MaterialData {
        public Color albedo;
        public string albedoMap;
        public double metallic;
        public string metallicMap;
        public double roughness;
        public string roughnessMap;
        public string normalMap;
        public string occlusionMap;
        public Color emissive;
        public string emissiveMap;
        
        public string getJSON() {
            string json = "{ \"materialVersion\": 1, \"materials\": { ";
            json += "\"albedo\": [" + albedo.r + ", " + albedo.g + ", " + albedo.b + "], ";
            if (!string.IsNullOrEmpty(albedoMap)) {
                json += "\"albedoMap\": \"" + albedoMap + "\", ";
            }
            json += "\"metallic\": " + metallic + ", ";
            if (!string.IsNullOrEmpty(metallicMap)) {
                json += "\"metallicMap\": \"" + metallicMap + "\", ";
            }
            json += "\"roughness\": " + roughness + ", ";
            if (!string.IsNullOrEmpty(roughnessMap)) {
                json += "\"roughnessMap\": \"" + roughnessMap + "\", ";
            }
            if (!string.IsNullOrEmpty(normalMap)) {
                json += "\"normalMap\": \"" + normalMap + "\", ";
            }
            if (!string.IsNullOrEmpty(occlusionMap)) {
                json += "\"occlusionMap\": \"" + occlusionMap + "\", ";
            }
            json += "\"emissive\": [" + emissive.r + ", " + emissive.g + ", " + emissive.b + "]";
            if (!string.IsNullOrEmpty(emissiveMap)) {
                json += ", \"emissiveMap\": \"" + emissiveMap + "\"";
            }
            json += " } }";
            return json;
        }
    }
    
    static string assetPath = "";
    static string assetName = "";    
    static ModelImporter modelImporter;
    static HumanDescription humanDescription;

    static Dictionary<string, UserBoneInformation> userBoneInfos = new Dictionary<string, UserBoneInformation>();
    static Dictionary<string, string> humanoidToUserBoneMappings = new Dictionary<string, string>();
    static BoneTreeNode userBoneTree = new BoneTreeNode();
    static Dictionary<AvatarRule, string> failedAvatarRules = new Dictionary<AvatarRule, string>();
    static string warnings = "";
    
    static Dictionary<string, string> textureDependencies = new Dictionary<string, string>();
    static Dictionary<string, string> materialMappings = new Dictionary<string, string>();
    static Dictionary<string, MaterialData> materialDatas = new Dictionary<string, MaterialData>();
    static List<string> alternateStandardShaderMaterials = new List<string>();
    static List<string> unsupportedShaderMaterials = new List<string>();
    
    static Scene previewScene;
    static string previousScene = "";
    static Vector3 previousScenePivot = Vector3.zero;
    static Quaternion previousSceneRotation = Quaternion.identity;
    static float previousSceneSize = 0.0f;
    static bool previousSceneOrthographic = false;
    static UnityEngine.Object avatarResource;
    static GameObject avatarPreviewObject;
    static GameObject heightReferenceObject;

    [MenuItem("High Fidelity/Export New Avatar")]
    static void ExportNewAvatar() {
        ExportSelectedAvatar(false);
    }

    [MenuItem("High Fidelity/Update Existing Avatar")]
    static void UpdateAvatar() {
        ExportSelectedAvatar(true);
    }
    
    [MenuItem("High Fidelity/About")]
    static void About() {
        EditorUtility.DisplayDialog("About", "High Fidelity, Inc.\nAvatar Exporter\nVersion " + AVATAR_EXPORTER_VERSION, "Ok");
    }

    static void ExportSelectedAvatar(bool updateExistingAvatar) {
        // ensure everything is saved to file before doing anything
        AssetDatabase.SaveAssets();
        
        string[] guids = Selection.assetGUIDs;
        if (guids.Length != 1) {
            if (guids.Length == 0) {
                EditorUtility.DisplayDialog("Error", "Please select an asset to export.", "Ok");
            } else {
                EditorUtility.DisplayDialog("Error", "Please select a single asset to export.", "Ok");
            }
            return;
        }
        assetPath = AssetDatabase.GUIDToAssetPath(Selection.assetGUIDs[0]);
        assetName = Path.GetFileNameWithoutExtension(assetPath);
        modelImporter = ModelImporter.GetAtPath(assetPath) as ModelImporter;
        if (Path.GetExtension(assetPath).ToLower() != ".fbx" || modelImporter == null) {
            EditorUtility.DisplayDialog("Error", "Please select an .fbx model asset to export.", "Ok");
            return;
        }
        if (modelImporter.animationType != ModelImporterAnimationType.Human) {
            EditorUtility.DisplayDialog("Error", "Please set model's Animation Type to Humanoid in " + 
                                                 " the Rig section of it's Inspector window.", "Ok");
            return;
        }
        
        avatarResource = AssetDatabase.LoadAssetAtPath(assetPath, typeof(UnityEngine.Object));
        humanDescription = modelImporter.humanDescription;
        
        string textureWarnings = SetTextureDependencies();

        // if the rig is optimized we should de-optimize it during the export process
        bool shouldDeoptimizeGameObjects = modelImporter.optimizeGameObjects;
        if (shouldDeoptimizeGameObjects) {
            modelImporter.optimizeGameObjects = false;
            modelImporter.SaveAndReimport();
        }
        
        SetBoneAndMaterialInformation();
        
        if (shouldDeoptimizeGameObjects) {
            // switch back to optimized game object in case it was originally optimized
            modelImporter.optimizeGameObjects = true;
            modelImporter.SaveAndReimport();
        }
        
        // check if we should be substituting a bone for a missing UpperChest mapping
        AdjustUpperChestMapping();

        // format resulting avatar rule failure strings
        // consider export-blocking avatar rules to be errors and show them in an error dialog,
        // and also include any other avatar rule failures plus texture warnings as warnings in the dialog
        string boneErrors = "";
        warnings = "";
        foreach (var failedAvatarRule in failedAvatarRules) {
            if (Array.IndexOf(EXPORT_BLOCKING_AVATAR_RULES, failedAvatarRule.Key) >= 0) {
                boneErrors += failedAvatarRule.Value + "\n\n";
            } else {
                warnings += failedAvatarRule.Value + "\n\n";
            }           
        }
        
        // add material and texture warnings after bone-related warnings
        AddMaterialWarnings();  
        warnings += textureWarnings;
        
        // remove trailing newlines at the end of the warnings
        if (!string.IsNullOrEmpty(warnings)) {
            warnings = warnings.Substring(0, warnings.LastIndexOf("\n\n"));
        }
        
        if (!string.IsNullOrEmpty(boneErrors)) {
            // if there are both errors and warnings then warnings will be displayed with errors in the error dialog
            if (!string.IsNullOrEmpty(warnings)) {
                boneErrors = "Errors:\n\n" + boneErrors;
                boneErrors += "Warnings:\n\n" + warnings;
            }
            // remove ending newlines from the last rule failure string that was added above
            boneErrors = boneErrors.Substring(0, boneErrors.LastIndexOf("\n\n"));
            EditorUtility.DisplayDialog("Error", boneErrors, "Ok");
            return;
        }
        
        // since there are no errors we can now open the preview scene in place of the user's scene
        if (!OpenPreviewScene()) {
            return;
        }
        
        // show None instead of blank warnings if there are no warnings in the export windows
        if (string.IsNullOrEmpty(warnings)) {
            warnings = EMPTY_WARNING_TEXT;
        }
        
        string documentsFolder = System.Environment.GetFolderPath(System.Environment.SpecialFolder.MyDocuments);
        string hifiFolder = documentsFolder + "\\High Fidelity Projects";
        if (updateExistingAvatar) { // Update Existing Avatar menu option           
            // open update existing project popup window including project to update, scale, and warnings
            // default the initial file chooser location to HiFi projects folder in user documents folder
            ExportProjectWindow window = ScriptableObject.CreateInstance<ExportProjectWindow>();
            string initialPath = Directory.Exists(hifiFolder) ? hifiFolder : documentsFolder;
            window.Init(initialPath, warnings, updateExistingAvatar, avatarPreviewObject, OnUpdateExistingProject, OnExportWindowClose);
        } else { // Export New Avatar menu option
            // create High Fidelity Projects folder in user documents folder if it doesn't exist
            if (!Directory.Exists(hifiFolder)) {    
                Directory.CreateDirectory(hifiFolder);
            }
            
            // open export new project popup window including project name, project location, scale, and warnings
            // default the initial project location path to the High Fidelity Projects folder above
            ExportProjectWindow window = ScriptableObject.CreateInstance<ExportProjectWindow>();
            window.Init(hifiFolder, warnings, updateExistingAvatar, avatarPreviewObject, OnExportNewProject, OnExportWindowClose);
        }
    }
    
    static void OnUpdateExistingProject(string exportFstPath, string projectName, float scale) {
        bool copyModelToExport = false;
        
        // lookup the project name field from the fst file to update
        projectName = "";
        try {
            string[] lines = File.ReadAllLines(exportFstPath);
            foreach (string line in lines) {
                int separatorIndex = line.IndexOf("=");
                if (separatorIndex >= 0) {
                    string key = line.Substring(0, separatorIndex).Trim();
                    if (key == "name") {
                        projectName = line.Substring(separatorIndex + 1).Trim();
                        break;
                    }
                }
            }
        } catch {
            EditorUtility.DisplayDialog("Error", "Failed to read from existing file " + exportFstPath + 
                                        ". Please check the file and try again.", "Ok");
            return;
        }
        
        string exportModelPath = Path.GetDirectoryName(exportFstPath) + "\\" + assetName + ".fbx";
        if (File.Exists(exportModelPath)) { 
            // if the fbx in Unity Assets is newer than the fbx in the target export
            // folder or vice-versa then ask to replace the older fbx with the newer fbx
            DateTime assetModelWriteTime = File.GetLastWriteTime(assetPath);
            DateTime targetModelWriteTime = File.GetLastWriteTime(exportModelPath);
            if (assetModelWriteTime > targetModelWriteTime) {
                int option = EditorUtility.DisplayDialogComplex("Error", "The " + assetName + 
                             ".fbx model in the Unity Assets folder is newer than the " + exportModelPath + 
                             " model.\n\nDo you want to replace the older .fbx with the newer .fbx?",
                             "Yes", "No", "Cancel");
                if (option == 2) { // Cancel
                    return;
                }
                copyModelToExport = option == 0; // Yes
            } else if (assetModelWriteTime < targetModelWriteTime) {
                int option = EditorUtility.DisplayDialogComplex("Error", "The " + exportModelPath + 
                             " model is newer than the " + assetName + ".fbx model in the Unity Assets folder." +
                             "\n\nDo you want to replace the older .fbx with the newer .fbx and re-import it?", 
                             "Yes", "No" , "Cancel");
                if (option == 2) { // Cancel
                    return;
                } else if (option == 0) { // Yes - copy model to Unity project
                    // copy the fbx from the project folder to Unity Assets, overwriting the existing fbx, and re-import it
                    try {
                        File.Copy(exportModelPath, assetPath, true);
                    } catch {
                        EditorUtility.DisplayDialog("Error", "Failed to copy existing file " + exportModelPath + " to " + assetPath + 
                                                    ". Please check the location and try again.", "Ok");
                        return;
                    }
                    AssetDatabase.ImportAsset(assetPath);
                    
                    // set model to Humanoid animation type and force another refresh on it to process Humanoid
                    modelImporter = ModelImporter.GetAtPath(assetPath) as ModelImporter;
                    modelImporter.animationType = ModelImporterAnimationType.Human;
                    EditorUtility.SetDirty(modelImporter);
                    modelImporter.SaveAndReimport(); 
                    
                    // redo parent names, joint mappings, and user bone positions due to the fbx change
                    // as well as re-check the avatar rules for failures
                    humanDescription = modelImporter.humanDescription;
                    SetBoneAndMaterialInformation();
                }
            }
        } else {
            // if no matching fbx exists in the target export folder then ask to copy fbx over
            int option = EditorUtility.DisplayDialogComplex("Error", "There is no existing " + exportModelPath + 
                         " model.\n\nDo you want to copy over the " + assetName + 
                         ".fbx model from the Unity Assets folder?", "Yes", "No", "Cancel");
            if (option == 2) { // Cancel
                return;
            }
            copyModelToExport = option == 0; // Yes
        }  

        // copy asset fbx over deleting any existing fbx if we agreed to overwrite it
        if (copyModelToExport) {
            try {
                File.Copy(assetPath, exportModelPath, true);
            } catch {
                EditorUtility.DisplayDialog("Error", "Failed to copy existing file " + assetPath + " to " + exportModelPath + 
                                            ". Please check the location and try again.", "Ok");
                return;
            }
        }
        
        // delete existing fst file since we will write a new file
        // TODO: updating fst should only rewrite joint mappings and joint rotation offsets to existing file
        try {
            File.Delete(exportFstPath);
        } catch {
            EditorUtility.DisplayDialog("Error", "Failed to overwrite existing file " + exportFstPath + 
                                        ". Please check the file and try again.", "Ok");
            return;
        }

        // write out a new fst file in place of the old file
        if (!WriteFST(exportFstPath, projectName, scale)) {
            return;
        }
        
        // copy any external texture files to the project's texture directory that are considered dependencies of the model
        string texturesDirectory = GetTextureDirectory(exportFstPath);
        if (!CopyExternalTextures(texturesDirectory)) {
            return;
        }
        
        // display success dialog with any avatar rule warnings
        string successDialog = "Avatar successfully updated!";
        if (!string.IsNullOrEmpty(warnings)) {
            successDialog += "\n\nWarnings:\n" + warnings;
        }
        EditorUtility.DisplayDialog("Success!", successDialog, "Ok");
    }
    
    static void OnExportNewProject(string projectDirectory, string projectName, float scale) {     
        // copy the fbx from the Unity Assets folder to the project directory
        string exportModelPath = projectDirectory + assetName + ".fbx";
        File.Copy(assetPath, exportModelPath);
        
        // create empty Textures and Scripts folders in the project directory
        string texturesDirectory = GetTextureDirectory(projectDirectory);
        string scriptsDirectory = projectDirectory + "\\scripts";
        Directory.CreateDirectory(texturesDirectory);
        Directory.CreateDirectory(scriptsDirectory);
        
        // write out the avatar.fst file to the project directory
        string exportFstPath = projectDirectory + "avatar.fst";
        if (!WriteFST(exportFstPath, projectName, scale)) {
            return;
        }
        
        // copy any external texture files to the project's texture directory that are considered dependencies of the model
        if (!CopyExternalTextures(texturesDirectory)) {
            return;
        }
        
        // remove any double slashes in texture directory path, display success dialog with any
        // bone warnings previously mentioned, and suggest user to copy external textures over
        string successDialog = "Avatar successfully exported!\n\n";
        if (warnings != EMPTY_WARNING_TEXT) {
            successDialog += "Warnings:\n" + warnings;
        }
        successDialog += "\n\nNote: If you are using any external textures with your model, " +
                         "please ensure those textures are copied to " + texturesDirectory;
        EditorUtility.DisplayDialog("Success!", successDialog, "Ok");
    }

    static void OnExportWindowClose() {
        // close the preview avatar scene and go back to user's previous scene when export project windows close
        ClosePreviewScene();
    }
    
    // The High Fidelity FBX Serializer omits the colon based prefixes. This will make the jointnames compatible.
    static string removeTypeFromJointname(string jointName) {
        return jointName.Substring(jointName.IndexOf(':') + 1);
    }
    
    static bool WriteFST(string exportFstPath, string projectName, float scale) {        
        // write out core fields to top of fst file
        try {
            File.WriteAllText(exportFstPath, "exporterVersion = " + AVATAR_EXPORTER_VERSION + "\nname = " + projectName + 
                                             "\ntype = body+head\nscale = " + scale + "\nfilename = " + assetName + 
                                             ".fbx\n" + "texdir = textures\n");
        } catch { 
            EditorUtility.DisplayDialog("Error", "Failed to write file " + exportFstPath + 
                                        ". Please check the location and try again.", "Ok");
            return false;
        }
        
        // write out joint mappings to fst file
        foreach (var userBoneInfo in userBoneInfos) {
            if (userBoneInfo.Value.HasHumanMapping()) {
                string hifiJointName = HUMANOID_TO_HIFI_JOINT_NAME[userBoneInfo.Value.humanName];
                File.AppendAllText(exportFstPath, "jointMap = " + hifiJointName + " = " + removeTypeFromJointname(userBoneInfo.Key) + "\n");
            }
        }
        
        // calculate and write out joint rotation offsets to fst file
        SkeletonBone[] skeletonMap = humanDescription.skeleton;
        foreach (SkeletonBone userBone in skeletonMap) {
            string userBoneName = userBone.name;
            UserBoneInformation userBoneInfo;
            if (!userBoneInfos.TryGetValue(userBoneName, out userBoneInfo)) {
                continue;
            }
            
            Quaternion userBoneRotation = userBone.rotation;   
            string parentName = userBoneInfo.parentName;
            if (parentName == "root") {
                // if the parent is root then use bone's rotation
                userBoneInfo.rotation = userBoneRotation;
            } else {
                // otherwise multiply bone's rotation by parent bone's absolute rotation
                userBoneInfo.rotation = userBoneInfos[parentName].rotation * userBoneRotation;
            }
            
            // generate joint rotation offsets for both humanoid-mapped bones as well as extra unmapped bones
            Quaternion jointOffset = new Quaternion();
            if (userBoneInfo.HasHumanMapping()) {
                Quaternion rotation = REFERENCE_ROTATIONS[userBoneInfo.humanName];
                jointOffset = Quaternion.Inverse(userBoneInfo.rotation) * rotation;
            } else {
                jointOffset = Quaternion.Inverse(userBoneInfo.rotation);
                string lastRequiredParent = FindLastRequiredAncestorBone(userBoneName);
                if (lastRequiredParent != "root") {
                    // take the previous offset and multiply it by the current local when we have an extra joint
                    string lastRequiredParentHumanName = userBoneInfos[lastRequiredParent].humanName;
                    Quaternion lastRequiredParentRotation = REFERENCE_ROTATIONS[lastRequiredParentHumanName];
                    jointOffset *= lastRequiredParentRotation;
                }
            }
            
            // swap from left-handed (Unity) to right-handed (HiFi) coordinates and write out joint rotation offset to fst
            jointOffset = new Quaternion(-jointOffset.x, jointOffset.y, jointOffset.z, -jointOffset.w);
            File.AppendAllText(exportFstPath, "jointRotationOffset2 = " + removeTypeFromJointname(userBoneName) + " = (" + jointOffset.x + ", " +
                                              jointOffset.y + ", " + jointOffset.z + ", " + jointOffset.w + ")\n");
        }
        
        // if there is any material data to save then write out all materials in JSON material format to the materialMap field
        if (materialDatas.Count > 0) {
            string materialJson = "{ ";
            foreach (var materialData in materialDatas) {
                // if this is the only material in the mapping and it is the default name No Name mapped to No Name, 
                // then the avatar has no embedded materials and this material should be applied to all meshes
                string materialName = materialData.Key;
                if (materialMappings.Count == 1 && materialName == DEFAULT_MATERIAL_NAME && 
                    materialMappings[materialName] == DEFAULT_MATERIAL_NAME) {
                    materialJson += "\"all\": ";
                } else {
                    materialJson += "\"mat::" + materialName + "\": ";
                }
                materialJson += materialData.Value.getJSON();
                materialJson += ", ";
            }
            materialJson = materialJson.Substring(0, materialJson.LastIndexOf(", "));
            materialJson += " }";
            File.AppendAllText(exportFstPath, "materialMap = " + materialJson);
        }
             
        // open File Explorer to the project directory once finished
        System.Diagnostics.Process.Start("explorer.exe", "/select," + exportFstPath);
        
        return true;
    }
    
    static void SetBoneAndMaterialInformation() {
        userBoneInfos.Clear();
        humanoidToUserBoneMappings.Clear();
        userBoneTree = new BoneTreeNode();
        
        materialDatas.Clear();
        alternateStandardShaderMaterials.Clear();
        unsupportedShaderMaterials.Clear();

        SetMaterialMappings();

        // instantiate a game object of the user avatar to traverse the bone tree to gather
        // bone parents and positions as well as build a bone tree, then destroy it   
        GameObject assetGameObject = (GameObject)Instantiate(avatarResource);     
        TraverseUserBoneTree(assetGameObject.transform);      
        DestroyImmediate(assetGameObject);
        
        // iterate over Humanoid bones and update user bone info to increase human mapping counts for each bone
        // as well as set their Humanoid name and build a Humanoid to user bone mapping
        HumanBone[] boneMap = humanDescription.human;
        foreach (HumanBone bone in boneMap) {
            string humanName = bone.humanName;
            string userBoneName = bone.boneName;
            string hifiJointName;
            if (userBoneInfos.ContainsKey(userBoneName)) {
                ++userBoneInfos[userBoneName].mappingCount;
                if (HUMANOID_TO_HIFI_JOINT_NAME.TryGetValue(humanName, out hifiJointName)) {
                    userBoneInfos[userBoneName].humanName = humanName;
                    humanoidToUserBoneMappings.Add(humanName, userBoneName);
                }
            }
        }
        
        // generate the list of avatar rule failure strings for any avatar rules that are not satisfied by this avatar
        SetFailedAvatarRules();
    }

    static void TraverseUserBoneTree(Transform modelBone) {
        GameObject gameObject = modelBone.gameObject;

        // check if this transform is a node containing mesh, light, or camera instead of a bone
        MeshRenderer meshRenderer = gameObject.GetComponent<MeshRenderer>();
        SkinnedMeshRenderer skinnedMeshRenderer = gameObject.GetComponent<SkinnedMeshRenderer>();
        bool mesh = meshRenderer != null || skinnedMeshRenderer != null;
        bool light = gameObject.GetComponent<Light>() != null;
        bool camera = gameObject.GetComponent<Camera>() != null;
        
        // if this is a mesh then store its material data to be exported if the material is mapped to an fbx material name
        if (mesh) {
            Material[] materials = skinnedMeshRenderer != null ? skinnedMeshRenderer.sharedMaterials : meshRenderer.sharedMaterials;
            StoreMaterialData(materials);
        } else if (!light && !camera) { 
            // if it is in fact a bone, add it to the bone tree as well as user bone infos list with position and parent name
            UserBoneInformation userBoneInfo = new UserBoneInformation();
            userBoneInfo.position = modelBone.position; // bone's absolute position
            
            string boneName = modelBone.name;
            if (modelBone.parent == null) {
                // if no parent then this is actual root bone node of the user avatar, so consider it's parent as "root"
                boneName = GetRootBoneName(); // ensure we use the root bone name from the skeleton list for consistency
                userBoneTree = new BoneTreeNode(boneName); // initialize root of tree
                userBoneInfo.parentName = "root";
                userBoneInfo.boneTreeNode = userBoneTree;
            } else {
                // otherwise add this bone node as a child to it's parent's children list
                // if its a child of the root bone, use the root bone name from the skeleton list as the parent for consistency
                string parentName = modelBone.parent.parent == null ? GetRootBoneName() : modelBone.parent.name;
                BoneTreeNode boneTreeNode = new BoneTreeNode(boneName);
                userBoneInfos[parentName].boneTreeNode.children.Add(boneTreeNode);
                userBoneInfo.parentName = parentName;
            }

            userBoneInfos.Add(boneName, userBoneInfo);
        }
        
        // recurse over transform node's children
        for (int i = 0; i < modelBone.childCount; ++i) {
            TraverseUserBoneTree(modelBone.GetChild(i));
        }
    }
    
    static string FindLastRequiredAncestorBone(string currentBone) {
        string result = currentBone;
        // iterating upward through user bone info parent names, find the first ancestor bone that is mapped in Humanoid
        while (result != "root" && userBoneInfos.ContainsKey(result) && !userBoneInfos[result].HasHumanMapping()) {
            result = userBoneInfos[result].parentName;            
        }
        return result;
    }

    static void AdjustUpperChestMapping() {
        if (!humanoidToUserBoneMappings.ContainsKey("UpperChest")) {
            // if parent of Neck is not Chest then map the parent to UpperChest
            string neckUserBone;
            if (humanoidToUserBoneMappings.TryGetValue("Neck", out neckUserBone)) {
                UserBoneInformation neckParentBoneInfo;
                string neckParentUserBone = userBoneInfos[neckUserBone].parentName;
                if (userBoneInfos.TryGetValue(neckParentUserBone, out neckParentBoneInfo) && !neckParentBoneInfo.HasHumanMapping()) {
                    neckParentBoneInfo.humanName = "UpperChest";
                    humanoidToUserBoneMappings.Add("UpperChest", neckParentUserBone);
                }
            }
            // if there is still no UpperChest bone but there is a Chest bone then we remap Chest to UpperChest 
            string chestUserBone;
            if (!humanoidToUserBoneMappings.ContainsKey("UpperChest") && 
                humanoidToUserBoneMappings.TryGetValue("Chest", out chestUserBone)) {       
                userBoneInfos[chestUserBone].humanName = "UpperChest";
                humanoidToUserBoneMappings.Remove("Chest");
                humanoidToUserBoneMappings.Add("UpperChest", chestUserBone);
            }
        }
    }
    
    static string GetRootBoneName() {
        // the "root" bone is the first element in the human skeleton bone list
        if (humanDescription.skeleton.Length > 0) {
            return humanDescription.skeleton[0].name;
        }
        return "";
    }
    
    static void SetFailedAvatarRules() {
        failedAvatarRules.Clear();
        
        string hipsUserBone = "";
        string spineUserBone = "";
        string chestUserBone = "";
        string headUserBone = "";
        
        Vector3 hipsPosition = new Vector3();
        
        // iterate over all avatar rules in order and add any rules that fail 
        // to the failed avatar rules map with appropriate error or warning text
        for (AvatarRule avatarRule = 0; avatarRule < AvatarRule.AvatarRuleEnd; ++avatarRule) { 
            switch (avatarRule) {
                case AvatarRule.RecommendedUnityVersion:
                    if (Array.IndexOf(RECOMMENDED_UNITY_VERSIONS, Application.unityVersion) == -1) {
                        failedAvatarRules.Add(avatarRule, "The current version of Unity is not one of the recommended Unity " +
                                                          "versions. If you are using a version of Unity later than 2018.2.12f1, " +
                                                          "it is recommended to apply Enforce T-Pose under the Pose dropdown " +
                                                          "in Humanoid configuration.");
                    }
                    break;
                case AvatarRule.SingleRoot:
                    // avatar rule fails if the root bone node has more than one child bone
                    if (userBoneTree.children.Count > 1) {
                        failedAvatarRules.Add(avatarRule, "There is more than one bone at the top level of the selected avatar's " +
                                                          "bone hierarchy. Please ensure all bones for Humanoid mappings are " +
                                                          "under the same bone hierarchy.");
                    }
                    break;
                case AvatarRule.NoDuplicateMapping:
                    // avatar rule fails if any user bone is mapped to more than one Humanoid bone
                    foreach (var userBoneInfo in userBoneInfos) {
                        string boneName = userBoneInfo.Key;
                        int mappingCount = userBoneInfo.Value.mappingCount;
                        if (mappingCount > 1) {
                            string text = "The " + boneName + " bone is mapped to more than one bone in Humanoid.";
                            if (failedAvatarRules.ContainsKey(avatarRule)) {
                                failedAvatarRules[avatarRule] += "\n" + text;
                            } else {
                                failedAvatarRules.Add(avatarRule, text);
                            }
                        }
                    }
                    break;
                case AvatarRule.NoAsymmetricalLegMapping:
                    CheckAsymmetricalMappingRule(avatarRule, LEG_MAPPING_SUFFIXES, "leg");
                    break;
                case AvatarRule.NoAsymmetricalArmMapping:
                    CheckAsymmetricalMappingRule(avatarRule, ARM_MAPPING_SUFFIXES, "arm");
                    break;
                case AvatarRule.NoAsymmetricalHandMapping:
                    CheckAsymmetricalMappingRule(avatarRule, HAND_MAPPING_SUFFIXES, "hand");
                    break;
                case AvatarRule.HipsMapped:
                    hipsUserBone = CheckHumanBoneMappingRule(avatarRule, "Hips");
                    break;
                case AvatarRule.SpineMapped:
                    spineUserBone = CheckHumanBoneMappingRule(avatarRule, "Spine");
                    break;
                case AvatarRule.SpineDescendantOfHips:
                    CheckUserBoneDescendantOfHumanRule(avatarRule, spineUserBone, "Hips");
                    break;
                case AvatarRule.ChestMapped:
                    if (!humanoidToUserBoneMappings.TryGetValue("Chest", out chestUserBone)) {
                        // check to see if there is a child of Spine that we can suggest to be mapped to Chest
                        string spineChild = "";
                        if (!string.IsNullOrEmpty(spineUserBone)) {
                            BoneTreeNode spineTreeNode = userBoneInfos[spineUserBone].boneTreeNode;
                            if (spineTreeNode.children.Count == 1) {
                                spineChild = spineTreeNode.children[0].boneName;
                            }
                        }
                        failedAvatarRules.Add(avatarRule, "There is no Chest bone mapped in Humanoid for the selected avatar.");
                        // if the only found child of Spine is not yet mapped then add it as a suggestion for Chest mapping 
                        if (!string.IsNullOrEmpty(spineChild) && !userBoneInfos[spineChild].HasHumanMapping()) {
                            failedAvatarRules[avatarRule] += " It is suggested that you map bone " + spineChild + 
                                                             " to Chest in Humanoid.";
                        }
                    }
                    break;
                case AvatarRule.ChestDescendantOfSpine:
                    CheckUserBoneDescendantOfHumanRule(avatarRule, chestUserBone, "Spine");
                    break;
                case AvatarRule.NeckMapped:
                    CheckHumanBoneMappingRule(avatarRule, "Neck");
                    break;
                case AvatarRule.HeadMapped:
                    headUserBone = CheckHumanBoneMappingRule(avatarRule, "Head");
                    break;
                case AvatarRule.HeadDescendantOfChest:
                    CheckUserBoneDescendantOfHumanRule(avatarRule, headUserBone, "Chest");
                    break;
                case AvatarRule.EyesMapped:
                    bool leftEyeMapped = humanoidToUserBoneMappings.ContainsKey("LeftEye");
                    bool rightEyeMapped = humanoidToUserBoneMappings.ContainsKey("RightEye");
                    if (!leftEyeMapped || !rightEyeMapped) {
                        if (leftEyeMapped && !rightEyeMapped) {
                            failedAvatarRules.Add(avatarRule, "There is no RightEye bone mapped in Humanoid " + 
                                                              "for the selected avatar.");
                        } else if (!leftEyeMapped && rightEyeMapped) {
                            failedAvatarRules.Add(avatarRule, "There is no LeftEye bone mapped in Humanoid " + 
                                                              "for the selected avatar.");
                        } else {
                            failedAvatarRules.Add(avatarRule, "There is no LeftEye or RightEye bone mapped in Humanoid " + 
                                                              "for the selected avatar.");
                        }
                    }
                    break;
                case AvatarRule.HipsNotOnGround:
                    // ensure the absolute Y position for the bone mapped to Hips (if its mapped) is at least HIPS_GROUND_MIN_Y
                    if (!string.IsNullOrEmpty(hipsUserBone)) {
                        UserBoneInformation hipsBoneInfo = userBoneInfos[hipsUserBone];
                        hipsPosition = hipsBoneInfo.position;
                        if (hipsPosition.y < HIPS_GROUND_MIN_Y) {
                            failedAvatarRules.Add(avatarRule, "The bone mapped to Hips in Humanoid (" + hipsUserBone + 
                                                              ") should not be at ground level.");
                        }
                    }
                    break;
                case AvatarRule.HipsSpineChestNotCoincident:
                    // ensure the bones mapped to Hips, Spine, and Chest are all not in the same position,
                    // check Hips to Spine and Spine to Chest lengths are within HIPS_SPINE_CHEST_MIN_SEPARATION
                    if (!string.IsNullOrEmpty(spineUserBone) && !string.IsNullOrEmpty(chestUserBone) && 
                        !string.IsNullOrEmpty(hipsUserBone)) {
                        UserBoneInformation spineBoneInfo = userBoneInfos[spineUserBone];
                        UserBoneInformation chestBoneInfo = userBoneInfos[chestUserBone];
                        Vector3 hipsToSpine = hipsPosition - spineBoneInfo.position;
                        Vector3 spineToChest = spineBoneInfo.position - chestBoneInfo.position;
                        if (hipsToSpine.magnitude < HIPS_SPINE_CHEST_MIN_SEPARATION && 
                            spineToChest.magnitude < HIPS_SPINE_CHEST_MIN_SEPARATION) {
                            failedAvatarRules.Add(avatarRule, "The bone mapped to Hips in Humanoid (" + hipsUserBone + 
                                                              "), the bone mapped to Spine in Humanoid (" + spineUserBone + 
                                                              "), and the bone mapped to Chest in Humanoid (" + chestUserBone + 
                                                              ") should not be coincidental.");
                        }
                    }
                    break;
                case AvatarRule.TotalBoneCountUnderLimit:
                    int userBoneCount = userBoneInfos.Count;
                    if (userBoneCount > MAXIMUM_USER_BONE_COUNT) {
                        failedAvatarRules.Add(avatarRule, "The total number of bones in the avatar (" + userBoneCount + 
                                                          ") exceeds the maximum bone limit (" + MAXIMUM_USER_BONE_COUNT + ").");
                    }
                    break;
            }
        }
    }
    
    static string CheckHumanBoneMappingRule(AvatarRule avatarRule, string humanBoneName) {
        string userBoneName = "";
        // avatar rule fails if bone is not mapped in Humanoid
        if (!humanoidToUserBoneMappings.TryGetValue(humanBoneName, out userBoneName)) {
            failedAvatarRules.Add(avatarRule, "There is no " + humanBoneName + 
                                              " bone mapped in Humanoid for the selected avatar.");
        }
        return userBoneName;
    }
    
    static void CheckUserBoneDescendantOfHumanRule(AvatarRule avatarRule, string userBoneName, string descendantOfHumanName) {
        if (string.IsNullOrEmpty(userBoneName)) {
            return;
        }
        
        string descendantOfUserBoneName = "";
        if (!humanoidToUserBoneMappings.TryGetValue(descendantOfHumanName, out descendantOfUserBoneName)) {
            return;
        }

        string userBone = userBoneName;
        string ancestorUserBone = "";
        UserBoneInformation userBoneInfo = new UserBoneInformation();
        // iterate upward from user bone through user bone info parent names until root 
        // is reached or the ancestor bone name matches the target descendant of name
        while (ancestorUserBone != "root") {
            if (userBoneInfos.TryGetValue(userBone, out userBoneInfo)) {
                ancestorUserBone = userBoneInfo.parentName;
                if (ancestorUserBone == descendantOfUserBoneName) {
                    return;
                }
                userBone = ancestorUserBone;
            } else {
                break;
            }
        }
        
        // avatar rule fails if no ancestor of given user bone matched the descendant of name (no early return)
        failedAvatarRules.Add(avatarRule, "The bone mapped to " + userBoneInfo.humanName + " in Humanoid (" + userBoneName + 
                                          ") is not a child of the bone mapped to " + descendantOfHumanName + " in Humanoid (" + 
                                          descendantOfUserBoneName + ").");
    }
    
    static void CheckAsymmetricalMappingRule(AvatarRule avatarRule, string[] mappingSuffixes, string appendage) {
        int leftCount = 0;
        int rightCount = 0;
        // add Left/Right to each mapping suffix to make Humanoid mapping names, 
        // and count the number of bones mapped in Humanoid on each side
        foreach (string mappingSuffix in mappingSuffixes) {
            string leftMapping = "Left" + mappingSuffix;
            string rightMapping = "Right" + mappingSuffix;
            if (humanoidToUserBoneMappings.ContainsKey(leftMapping)) {
                ++leftCount;
            }
            if (humanoidToUserBoneMappings.ContainsKey(rightMapping)) {
                ++rightCount;
            }
        }
        // avatar rule fails if number of left appendage mappings doesn't match number of right appendage mappings 
        if (leftCount != rightCount) {
            failedAvatarRules.Add(avatarRule, "The number of bones mapped in Humanoid for the left " + appendage + " (" +
                                              leftCount + ") does not match the number of bones mapped in Humanoid for the right " +
                                              appendage + " (" + rightCount + ").");
        }
    }
    
    static string GetTextureDirectory(string basePath) {
        string textureDirectory = Path.GetDirectoryName(basePath) + "\\" + TEXTURES_DIRECTORY;
        textureDirectory = textureDirectory.Replace("\\\\", "\\");
        return textureDirectory;
    }

    static string SetTextureDependencies() {
        string textureWarnings = "";
        textureDependencies.Clear();
        
        // build the list of all local asset paths for textures that Unity considers dependencies of the model
        // for any textures that have duplicate names, return a string of duplicate name warnings
        string[] dependencies = AssetDatabase.GetDependencies(assetPath);
        foreach (string dependencyPath in dependencies) {
            UnityEngine.Object textureObject = AssetDatabase.LoadAssetAtPath(dependencyPath, typeof(Texture2D));
            if (textureObject != null) {
                string textureName = Path.GetFileName(dependencyPath);
                if (textureDependencies.ContainsKey(textureName)) {
                    textureWarnings += "There is more than one texture with the name " + textureName + 
                                       " referenced in the selected avatar.\n\n";
                } else {
                    textureDependencies.Add(textureName, dependencyPath);
                }
            }
        }
        
        return textureWarnings;
    }
    
    static bool CopyExternalTextures(string texturesDirectory) {
        // copy the found dependency textures from the local asset folder to the textures folder in the target export project
        foreach (var texture in textureDependencies) {
            string targetPath = texturesDirectory + "\\" + texture.Key;
            try {
                File.Copy(texture.Value, targetPath, true);
            } catch {
                EditorUtility.DisplayDialog("Error", "Failed to copy texture file " + texture.Value + " to " + targetPath +
                                            ". Please check the location and try again.", "Ok");
                return false;
            }
        }
        return true;
    }
    
    static void StoreMaterialData(Material[] materials) {
        // store each material's info in the materialDatas list to be written out later to the FST if it is a supported shader
        foreach (Material material in materials) {          
            string materialName = material.name;
            string shaderName = material.shader.name;
            
            // don't store any material data for unsupported shader types
            if (Array.IndexOf(SUPPORTED_SHADERS, shaderName) == -1) {
                if (!unsupportedShaderMaterials.Contains(materialName)) {
                    unsupportedShaderMaterials.Add(materialName);
                }
                continue;
            }           

            MaterialData materialData = new MaterialData();
            materialData.albedo = material.GetColor("_Color");
            materialData.albedoMap = GetMaterialTexture(material, "_MainTex");
            materialData.roughness = material.GetFloat("_Glossiness");
            materialData.roughnessMap = GetMaterialTexture(material, "_SpecGlossMap");
            materialData.normalMap = GetMaterialTexture(material, "_BumpMap");
            materialData.occlusionMap = GetMaterialTexture(material, "_OcclusionMap");
            materialData.emissive = material.GetColor("_EmissionColor");
            materialData.emissiveMap = GetMaterialTexture(material, "_EmissionMap");
            
            // for specular setups we will treat the metallic value as the average of the specular RGB intensities
            if (shaderName == STANDARD_SPECULAR_SHADER) {
                Color specular = material.GetColor("_SpecColor");
                materialData.metallic = (specular.r + specular.g + specular.b) / 3.0f;
            } else {
                materialData.metallic = material.GetFloat("_Metallic");
                materialData.metallicMap = GetMaterialTexture(material, "_MetallicGlossMap");
            }
            
            // for non-roughness Standard shaders give a warning that is not the recommended Standard shader, 
            // and invert smoothness for roughness
            if (shaderName == STANDARD_SHADER || shaderName == STANDARD_SPECULAR_SHADER) {
                if (!alternateStandardShaderMaterials.Contains(materialName)) {
                    alternateStandardShaderMaterials.Add(materialName);
                }
                materialData.roughness = 1.0f - materialData.roughness;
            }
            
            // store the material data under each fbx material name that it overrides from the material mapping
            foreach (var materialMapping in materialMappings) {
                string fbxMaterialName = materialMapping.Key;
                string unityMaterialName = materialMapping.Value;
                if (unityMaterialName == materialName && !materialDatas.ContainsKey(fbxMaterialName)) {
                    materialDatas.Add(fbxMaterialName, materialData);
                }
            }
        }
    }
    
    static string GetMaterialTexture(Material material, string textureProperty) {
        // ensure the texture property name exists in this material and return its texture directory path if so
        string[] textureNames = material.GetTexturePropertyNames();
        if (Array.IndexOf(textureNames, textureProperty) >= 0) {
            Texture texture = material.GetTexture(textureProperty);
            if (texture) {
                foreach (var textureDependency in textureDependencies) {
                    string textureFile = textureDependency.Key;
                    if (Path.GetFileNameWithoutExtension(textureFile) == texture.name) {
                        return TEXTURES_DIRECTORY + "/" + textureFile;
                    }
                }
            }
        }
        return "";
    }
    
    static void SetMaterialMappings() {
        materialMappings.Clear();
        
        // store the mappings from fbx material name to the Unity Material name that overrides it using external fbx mapping
        var objectMap = modelImporter.GetExternalObjectMap();
        foreach (var mapping in objectMap) {
            var material = mapping.Value as UnityEngine.Material;
            if (material != null) {
                materialMappings.Add(mapping.Key.name, material.name);
            }
        }
    }
    
    static void AddMaterialWarnings() {
        string alternateStandardShaders = "";
        string unsupportedShaders = "";
        // combine all material names for each material warning into a comma-separated string
        foreach (string materialName in alternateStandardShaderMaterials) {
            if (!string.IsNullOrEmpty(alternateStandardShaders)) {
                alternateStandardShaders += ", ";
            }
            alternateStandardShaders += materialName;
        }
        foreach (string materialName in unsupportedShaderMaterials) {
            if (!string.IsNullOrEmpty(unsupportedShaders)) {
                unsupportedShaders += ", ";
            }
            unsupportedShaders += materialName;
        }
        if (alternateStandardShaderMaterials.Count > 1) {
            warnings += "The materials " + alternateStandardShaders + " are not using the " +
                        "recommended variation of the Standard shader. We recommend you change " +
                        "them to Standard (Roughness setup) shader for improved performance.\n\n";
        } else if (alternateStandardShaderMaterials.Count == 1) {
            warnings += "The material " + alternateStandardShaders + " is not using the " +
                        "recommended variation of the Standard shader. We recommend you change " +
                        "it to Standard (Roughness setup) shader for improved performance.\n\n";
        }
        if (unsupportedShaderMaterials.Count > 1) {
            warnings += "The materials " + unsupportedShaders + " are using an unsupported shader. " +
                        "Please change them to a Standard shader type.\n\n";
        } else if (unsupportedShaderMaterials.Count == 1) {
            warnings += "The material " + unsupportedShaders + " is using an unsupported shader. " +
                        "Please change it to a Standard shader type.\n\n";
        }
    }
    
    static bool OpenPreviewScene() {
        // see if the user wants to save their current scene before opening preview avatar scene in place of user's scene
        if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) {
            return false;
        }
        
        // store the user's current scene to re-open when done and open a new default scene in place of the user's scene
        previousScene = EditorSceneManager.GetActiveScene().path;
        previewScene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene);
        
        // instantiate a game object to preview the avatar and a game object for the height reference prefab at 0, 0, 0
        UnityEngine.Object heightReferenceResource = AssetDatabase.LoadAssetAtPath(HEIGHT_REFERENCE_PREFAB, typeof(UnityEngine.Object));
        avatarPreviewObject = (GameObject)Instantiate(avatarResource, Vector3.zero, Quaternion.identity);
        heightReferenceObject = (GameObject)Instantiate(heightReferenceResource, Vector3.zero, Quaternion.identity);
        
        // store the camera pivot and rotation from the user's last scene to be restored later
        // replace the camera pivot and rotation to point at the preview avatar object in the -Z direction (facing front of it)
        var sceneView = SceneView.lastActiveSceneView;
        if (sceneView != null) {
            previousScenePivot = sceneView.pivot;
            previousSceneRotation = sceneView.rotation;
            previousSceneSize = sceneView.size;
            previousSceneOrthographic = sceneView.orthographic;
            sceneView.pivot = PREVIEW_CAMERA_PIVOT;
            sceneView.rotation = Quaternion.LookRotation(PREVIEW_CAMERA_DIRECTION);
            sceneView.orthographic = true;
            sceneView.size = 5.0f;
        }
        
        return true;
    }
    
    static void ClosePreviewScene() {
        // destroy the avatar and height reference game objects closing the scene
        DestroyImmediate(avatarPreviewObject);
        DestroyImmediate(heightReferenceObject);
        
        // re-open the scene the user had open before switching to the preview scene
        if (!string.IsNullOrEmpty(previousScene)) {
            EditorSceneManager.OpenScene(previousScene);
        }
        
        // close the preview scene and flag it to be removed
        EditorSceneManager.CloseScene(previewScene, true);
        
        // restore the camera pivot and rotation to the user's previous scene settings
        var sceneView = SceneView.lastActiveSceneView;
        if (sceneView != null) {
            sceneView.pivot = previousScenePivot;
            sceneView.rotation = previousSceneRotation;
            sceneView.size = previousSceneSize;
            sceneView.orthographic = previousSceneOrthographic;
        }
    }
}

class ExportProjectWindow : EditorWindow {
    const int WINDOW_WIDTH = 500;
    const int EXPORT_NEW_WINDOW_HEIGHT = 520;
    const int UPDATE_EXISTING_WINDOW_HEIGHT = 465;
    const int BUTTON_FONT_SIZE = 16;
    const int LABEL_FONT_SIZE = 16;
    const int TEXT_FIELD_FONT_SIZE = 14;
    const int TEXT_FIELD_HEIGHT = 20;
    const int ERROR_FONT_SIZE = 12;
    const int WARNING_SCROLL_HEIGHT = 170;
    const string EMPTY_ERROR_TEXT = "None\n";
    const int SLIDER_WIDTH = 340;
    const int SCALE_TEXT_WIDTH = 60;
    const float MIN_SCALE_SLIDER = 0.0f;
    const float MAX_SCALE_SLIDER = 2.0f;
    const int SLIDER_SCALE_EXPONENT = 10;
    const float ACTUAL_SCALE_OFFSET = 1.0f;
    const float DEFAULT_AVATAR_HEIGHT = 1.755f;
    const float MAXIMUM_RECOMMENDED_HEIGHT = DEFAULT_AVATAR_HEIGHT * 1.5f;
    const float MINIMUM_RECOMMENDED_HEIGHT = DEFAULT_AVATAR_HEIGHT * 0.25f;
    readonly Color COLOR_YELLOW = Color.yellow; //new Color(0.9176f, 0.8274f, 0.0f);
    readonly Color COLOR_BACKGROUND = new Color(0.5f, 0.5f, 0.5f);

    GameObject avatarPreviewObject;
    bool updateExistingAvatar = false;
    string projectName = "";
    string projectLocation = "";
    string initialProjectLocation = "";
    string projectDirectory = "";
    string errorText = EMPTY_ERROR_TEXT;
    string warningText = "\n";
    Vector2 warningScrollPosition = new Vector2(0, 0);
    string scaleWarningText = "";
    float sliderScale = 0.30103f;
    
    public delegate void OnExportDelegate(string projectDirectory, string projectName, float scale);
    OnExportDelegate onExportCallback;
    
    public delegate void OnCloseDelegate();
    OnCloseDelegate onCloseCallback;
    
    public void Init(string initialPath, string warnings, bool updateExisting, GameObject avatarObject,
                     OnExportDelegate exportCallback, OnCloseDelegate closeCallback) {
        updateExistingAvatar = updateExisting;
        float windowHeight = updateExistingAvatar ? UPDATE_EXISTING_WINDOW_HEIGHT : EXPORT_NEW_WINDOW_HEIGHT;
        minSize = new Vector2(WINDOW_WIDTH, windowHeight);
        maxSize = new Vector2(WINDOW_WIDTH, windowHeight);
        avatarPreviewObject = avatarObject;
        titleContent.text = updateExistingAvatar ? "Update Existing Avatar" : "Export New Avatar";
        initialProjectLocation = initialPath;
        projectLocation = updateExistingAvatar ? "" : initialProjectLocation;
        warningText = warnings;
        onExportCallback = exportCallback;
        onCloseCallback = closeCallback;
        
        ShowUtility();
        
        // if the avatar's starting height is outside of the recommended ranges, auto-adjust the scale to default height
        float height = GetAvatarHeight();
        if (height < MINIMUM_RECOMMENDED_HEIGHT || height > MAXIMUM_RECOMMENDED_HEIGHT) {
            float newScale = DEFAULT_AVATAR_HEIGHT / height;
            SetAvatarScale(newScale);
            scaleWarningText = "Avatar's scale automatically adjusted to be within the recommended range.";
        }
    }

    void OnGUI() {      
        // define UI styles for all GUI elements to be created
        GUIStyle buttonStyle = new GUIStyle(GUI.skin.button);
        buttonStyle.fontSize = BUTTON_FONT_SIZE;
        GUIStyle labelStyle = new GUIStyle(GUI.skin.label);
        labelStyle.fontSize = LABEL_FONT_SIZE;
        GUIStyle textStyle = new GUIStyle(GUI.skin.textField);
        textStyle.fontSize = TEXT_FIELD_FONT_SIZE;
        textStyle.fixedHeight = TEXT_FIELD_HEIGHT;
        GUIStyle errorStyle = new GUIStyle(GUI.skin.label); 
        errorStyle.fontSize = ERROR_FONT_SIZE;
        errorStyle.normal.textColor = Color.red;
        errorStyle.wordWrap = true;
        GUIStyle warningStyle = new GUIStyle(errorStyle); 
        warningStyle.normal.textColor = COLOR_YELLOW;
        GUIStyle sliderStyle = new GUIStyle(GUI.skin.horizontalSlider);
        sliderStyle.fixedWidth = SLIDER_WIDTH;
        GUIStyle sliderThumbStyle = new GUIStyle(GUI.skin.horizontalSliderThumb);
        
        // set the background for the window to a darker gray
        Texture2D backgroundTexture = new Texture2D(1, 1, TextureFormat.RGBA32, false);
        backgroundTexture.SetPixel(0, 0, COLOR_BACKGROUND);
        backgroundTexture.Apply();
        GUI.DrawTexture(new Rect(0, 0, maxSize.x, maxSize.y), backgroundTexture, ScaleMode.StretchToFill);
     
        GUILayout.Space(10);
        
        if (updateExistingAvatar) {
            // Project file to update label and input text field
            GUILayout.Label("Project file to update:", labelStyle);
            projectLocation = GUILayout.TextField(projectLocation, textStyle);
        } else {
            // Project name label and input text field
            GUILayout.Label("Export project name:", labelStyle);
            projectName = GUILayout.TextField(projectName, textStyle);
            
            GUILayout.Space(10);
            
            // Project location label and input text field
            GUILayout.Label("Export project location:", labelStyle);
            projectLocation = GUILayout.TextField(projectLocation, textStyle);
        }       
        
        // Browse button to open file/folder explorer and set project location
        if (GUILayout.Button("Browse", buttonStyle)) {
            string result = "";
            if (updateExistingAvatar) {
                // open file explorer starting at hifi projects folder in user documents and select target fst to update
                string initialPath = string.IsNullOrEmpty(projectLocation) ? initialProjectLocation : projectLocation;
                result = EditorUtility.OpenFilePanel("Select .fst to update", initialPath, "fst");
            } else {
                // open folder explorer starting at project location path and select folder to create project folder in
                result = EditorUtility.OpenFolderPanel("Select export location", projectLocation, "");
            }
            if (!string.IsNullOrEmpty(result)) { // file/folder selection not cancelled
                projectLocation = result.Replace('/', '\\');
            }
        }
        
        // warning if scale is above/below recommended range or if scale was auto-adjusted initially
        GUILayout.Label(scaleWarningText, warningStyle);
        
        // from left to right show scale label, scale slider itself, and scale value input with % value
        // slider value itself is from 0.0 to 2.0, and actual scale is an exponent of it with an offset of 1
        // displayed scale is the actual scale value with 2 decimal places, and changing the displayed
        // scale via keyboard does the inverse calculation to get the slider value via logarithm
        GUILayout.BeginHorizontal();
        GUILayout.Label("Scale:", labelStyle);
        sliderScale = GUILayout.HorizontalSlider(sliderScale, MIN_SCALE_SLIDER, MAX_SCALE_SLIDER, sliderStyle, sliderThumbStyle);
        float actualScale = (Mathf.Pow(SLIDER_SCALE_EXPONENT, sliderScale) - ACTUAL_SCALE_OFFSET);
        GUIStyle scaleInputStyle = new GUIStyle(textStyle);
        scaleInputStyle.fixedWidth = SCALE_TEXT_WIDTH;
        actualScale *= 100.0f; // convert to 100-based percentage for display purposes
        string actualScaleStr = GUILayout.TextField(String.Format("{0:0.00}", actualScale), scaleInputStyle);
        actualScaleStr = Regex.Replace(actualScaleStr, @"[^0-9.]", "");
        actualScale = float.Parse(actualScaleStr);
        actualScale /= 100.0f; // convert back to 1.0-based percentage
        SetAvatarScale(actualScale);
        GUILayout.Label("%", labelStyle);
        GUILayout.EndHorizontal();
        
        GUILayout.Space(15);
        
        // red error label text to display any file-related errors
        GUILayout.Label("Error:", errorStyle);
        GUILayout.Label(errorText, errorStyle);
        
        GUILayout.Space(10);
        
        // yellow warning label text to display scrollable list of any bone-related warnings 
        GUILayout.Label("Warnings:", warningStyle);
        warningScrollPosition = GUILayout.BeginScrollView(warningScrollPosition, GUILayout.Width(WINDOW_WIDTH), 
                                                          GUILayout.Height(WARNING_SCROLL_HEIGHT));
        GUILayout.Label(warningText, warningStyle);
        GUILayout.EndScrollView();
        
        GUILayout.Space(10);
        
        // export button will verify target project folder can actually be created (or target fst file is valid)
        // before closing popup window and calling back to initiate the export
        bool export = false;
        if (GUILayout.Button("Export", buttonStyle)) {
            export = true;
            if (!CheckForErrors(true)) {
                Close();
                onExportCallback(updateExistingAvatar ? projectLocation : projectDirectory, projectName, actualScale);
            }
        }
        
        // cancel button closes the popup window triggering the close callback to close the preview scene
        if (GUILayout.Button("Cancel", buttonStyle)) {
            Close();
        }
        
        // when any value changes check for any errors and update scale warning if we are not exporting
        if (GUI.changed && !export) {
            CheckForErrors(false);
            UpdateScaleWarning();
        }
    }
    
    bool CheckForErrors(bool exporting) {   
        errorText = EMPTY_ERROR_TEXT; // default to None if no errors found
        if (updateExistingAvatar) {
            // if any text is set in the project file to update field verify that the file actually exists
            if (projectLocation.Length > 0) {
                if (!File.Exists(projectLocation)) {
                    errorText = "Please select a valid project file to update.\n";
                    return true;
                }
            } else if (exporting) {
                errorText = "Please select a project file to update.\n";
                return true;
            }
        } else {
            projectDirectory = projectLocation + "\\" + projectName + "\\";
            if (projectName.Length > 0) {
                // new project must have a unique folder name since the folder will be created for it
                if (Directory.Exists(projectDirectory)) {
                    errorText = "A folder with the name " + projectName + 
                                 " already exists at that location.\nPlease choose a different project name or location.";
                    return true;
                }
            }
            if (projectLocation.Length > 0) {
                // before clicking Export we can verify that the project location at least starts with a drive
                if (!Char.IsLetter(projectLocation[0]) || projectLocation.Length == 1 || projectLocation[1] != ':') {
                    errorText = "Project location is invalid. Please choose a different project location.\n";
                    return true;
                }
            }
            if (exporting) {
                // when exporting, project name and location must both be defined, and project location must
                // be valid and accessible (we attempt to create the project folder at this time to verify this)
                if (projectName.Length == 0) {
                    errorText = "Please define a project name.\n";
                    return true;
                } else if (projectLocation.Length == 0) {
                    errorText = "Please define a project location.\n";
                    return true;
                } else {
                    try {
                        Directory.CreateDirectory(projectDirectory);
                    } catch {
                        errorText = "Project location is invalid. Please choose a different project location.\n";
                        return true;
                    }
                }
            }
        }
        
        return false;
    }
    
    void UpdateScaleWarning() {
        // called on any input changes
        float height = GetAvatarHeight();
        if (height < MINIMUM_RECOMMENDED_HEIGHT) {
            scaleWarningText = "The height of the avatar is below the recommended minimum.";
        } else if (height > MAXIMUM_RECOMMENDED_HEIGHT) {
            scaleWarningText = "The height of the avatar is above the recommended maximum.";
        } else {
            scaleWarningText = "";
        }
    }
    
    float GetAvatarHeight() {
        // height of an avatar model can be determined to be the max Y extents of the combined bounds for all its mesh renderers
        Bounds bounds = new Bounds();
        var meshRenderers = avatarPreviewObject.GetComponentsInChildren<MeshRenderer>();
        var skinnedMeshRenderers = avatarPreviewObject.GetComponentsInChildren<SkinnedMeshRenderer>();
        foreach (var renderer in meshRenderers) {
            bounds.Encapsulate(renderer.bounds);
        }
        foreach (var renderer in skinnedMeshRenderers) {
            bounds.Encapsulate(renderer.bounds);
        }
        return bounds.max.y;
    }
    
    void SetAvatarScale(float actualScale) {
        // set the new scale uniformly on the preview avatar's transform to show the resulting avatar size
        avatarPreviewObject.transform.localScale = new Vector3(actualScale, actualScale, actualScale);    
        
        // adjust slider scale value to match the new actual scale value
        sliderScale = GetSliderScaleFromActualScale(actualScale);
    }
    
    float GetSliderScaleFromActualScale(float actualScale) {
        // since actual scale is an exponent of slider scale with an offset, do the logarithm operation to convert it back
        return Mathf.Log(actualScale + ACTUAL_SCALE_OFFSET, SLIDER_SCALE_EXPONENT);
    }
    
    void OnDestroy() {
        onCloseCallback();
    }
}
