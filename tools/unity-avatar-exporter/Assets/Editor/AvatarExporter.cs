//  AvatarExporter.cs
//
//  Created by David Back on 28 Nov 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

using UnityEngine;
using UnityEditor;
using System;
using System.IO;
using System.Collections.Generic;

class AvatarExporter : MonoBehaviour {
    // update version number for every PR that changes this file, also set updated version in README file
    static readonly string AVATAR_EXPORTER_VERSION = "0.2";
    
    static readonly float HIPS_GROUND_MIN_Y = 0.01f;
    static readonly float HIPS_SPINE_CHEST_MIN_SEPARATION = 0.001f;
    static readonly int MAXIMUM_USER_BONE_COUNT = 256;
    static readonly string EMPTY_WARNING_TEXT = "None";
    
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

    enum BoneRule {
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
        BoneRuleEnd,
    };
    // rules that are treated as errors and prevent exporting, otherwise rules will show as warnings
    static readonly BoneRule[] EXPORT_BLOCKING_BONE_RULES = new BoneRule[] {
        BoneRule.HipsMapped,
        BoneRule.SpineMapped,
        BoneRule.ChestMapped,
        BoneRule.HeadMapped,
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
    
    static Dictionary<string, UserBoneInformation> userBoneInfos = new Dictionary<string, UserBoneInformation>();
    static Dictionary<string, string> humanoidToUserBoneMappings = new Dictionary<string, string>();
    static BoneTreeNode userBoneTree = new BoneTreeNode();
    static Dictionary<BoneRule, string> failedBoneRules = new Dictionary<BoneRule, string>();
    
    static string assetPath = "";
    static string assetName = "";
    static HumanDescription humanDescription;
    static Dictionary<string, string> dependencyTextures = new Dictionary<string, string>();

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

    static void ExportSelectedAvatar(bool updateAvatar) {     
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
        ModelImporter modelImporter = ModelImporter.GetAtPath(assetPath) as ModelImporter;
        if (Path.GetExtension(assetPath).ToLower() != ".fbx" || modelImporter == null) {
            EditorUtility.DisplayDialog("Error", "Please select an .fbx model asset to export.", "Ok");
            return;
        }
        if (modelImporter.animationType != ModelImporterAnimationType.Human) {
            EditorUtility.DisplayDialog("Error", "Please set model's Animation Type to Humanoid in " + 
                                                 " the Rig section of it's Inspector window.", "Ok");
            return;
        }

        humanDescription = modelImporter.humanDescription;
        SetUserBoneInformation();
        string textureWarnings = SetTextureDependencies();
        
        // generate the list of bone rule failure strings for any bone rules that are not satisfied by this avatar
        SetFailedBoneRules();
        
        // check if we should be substituting a bone for a missing UpperChest mapping
        AdjustUpperChestMapping();

        // format resulting bone rule failure strings
        // consider export-blocking bone rules to be errors and show them in an error dialog,
        // and also include any other bone rule failures plus texture warnings as warnings in the dialog
        string boneErrors = "";
        string warnings = "";
        foreach (var failedBoneRule in failedBoneRules) {
            if (Array.IndexOf(EXPORT_BLOCKING_BONE_RULES, failedBoneRule.Key) >= 0) {
                boneErrors += failedBoneRule.Value + "\n\n";
            } else {
                warnings += failedBoneRule.Value + "\n\n";
            }           
        }
        warnings += textureWarnings;
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

        string documentsFolder = System.Environment.GetFolderPath(System.Environment.SpecialFolder.MyDocuments);
        string hifiFolder = documentsFolder + "\\High Fidelity Projects";
        if (updateAvatar) { // Update Existing Avatar menu option
            bool copyModelToExport = false;
            string initialPath = Directory.Exists(hifiFolder) ? hifiFolder : documentsFolder;
            
            // open file explorer defaulting to hifi projects folder in user documents to select target fst to update
            string exportFstPath = EditorUtility.OpenFilePanel("Select .fst to update", initialPath, "fst");
            if (exportFstPath.Length == 0) { // file selection cancelled
                return;
            }
            exportFstPath = exportFstPath.Replace('/', '\\');
            
            // lookup the project name field from the fst file to update
            string projectName = "";
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
                        // as well as re-check the bone rules for failures
                        humanDescription = modelImporter.humanDescription;
                        SetUserBoneInformation();
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
            if (!WriteFST(exportFstPath, projectName)) {
                return;
            }
            
            // copy any external texture files to the project's texture directory that are considered dependencies of the model
            string texturesDirectory = Path.GetDirectoryName(exportFstPath) + "\\textures";
            if (!CopyExternalTextures(texturesDirectory)) {
                return;
            }
            
            // display success dialog with any bone rule warnings
            string successDialog = "Avatar successfully updated!";
            if (!string.IsNullOrEmpty(warnings)) {
                successDialog += "\n\nWarnings:\n" + warnings;
            }
            EditorUtility.DisplayDialog("Success!", successDialog, "Ok");
        } else { // Export New Avatar menu option
            // create High Fidelity Projects folder in user documents folder if it doesn't exist
            if (!Directory.Exists(hifiFolder)) {    
                Directory.CreateDirectory(hifiFolder);
            }
            
            if (string.IsNullOrEmpty(warnings)) {
                warnings = EMPTY_WARNING_TEXT;
            }
            
            // open a popup window to enter new export project name and project location
            ExportProjectWindow window = ScriptableObject.CreateInstance<ExportProjectWindow>();
            window.Init(hifiFolder, warnings, OnExportProjectWindowClose);
        }
    }
    
    static void OnExportProjectWindowClose(string projectDirectory, string projectName, string warnings) {
        // copy the fbx from the Unity Assets folder to the project directory
        string exportModelPath = projectDirectory + assetName + ".fbx";
        File.Copy(assetPath, exportModelPath);
        
        // create empty Textures and Scripts folders in the project directory
        string texturesDirectory = projectDirectory + "\\textures";
        string scriptsDirectory = projectDirectory + "\\scripts";
        Directory.CreateDirectory(texturesDirectory);
        Directory.CreateDirectory(scriptsDirectory);
        
        // write out the avatar.fst file to the project directory
        string exportFstPath = projectDirectory + "avatar.fst";
        if (!WriteFST(exportFstPath, projectName)) {
            return;
        }
        
        // copy any external texture files to the project's texture directory that are considered dependencies of the model
        texturesDirectory = texturesDirectory.Replace("\\\\", "\\");
        if (!CopyExternalTextures(texturesDirectory)) {
            return;
        }
        
        // remove any double slashes in texture directory path, display success dialog with any
        // bone warnings previously mentioned, and suggest user to copy external textures over
        string successDialog = "Avatar successfully exported!\n\n";
        if (warnings != EMPTY_WARNING_TEXT) {
            successDialog += "Warnings:\n" + warnings;
        }
        successDialog += "Note: If you are using any external textures with your model, " +
                         "please ensure those textures are copied to " + texturesDirectory;
        EditorUtility.DisplayDialog("Success!", successDialog, "Ok");
    }
    
    static bool WriteFST(string exportFstPath, string projectName) {        
        // write out core fields to top of fst file
        try {
            File.WriteAllText(exportFstPath, "name = " + projectName + "\ntype = body+head\nscale = 1\nfilename = " + 
                              assetName + ".fbx\n" + "texdir = textures\n");
        } catch { 
            EditorUtility.DisplayDialog("Error", "Failed to write file " + exportFstPath + 
                                        ". Please check the location and try again.", "Ok");
            return false;
        }
        
        // write out joint mappings to fst file
        foreach (var userBoneInfo in userBoneInfos) {
            if (userBoneInfo.Value.HasHumanMapping()) {
                string hifiJointName = HUMANOID_TO_HIFI_JOINT_NAME[userBoneInfo.Value.humanName];
                File.AppendAllText(exportFstPath, "jointMap = " + hifiJointName + " = " + userBoneInfo.Key + "\n");
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
            string outputJointName = "";
            if (userBoneInfo.HasHumanMapping()) {
                outputJointName = HUMANOID_TO_HIFI_JOINT_NAME[userBoneInfo.humanName];
                Quaternion rotation = REFERENCE_ROTATIONS[userBoneInfo.humanName];
                jointOffset = Quaternion.Inverse(userBoneInfo.rotation) * rotation;
            } else {
                outputJointName = userBoneName;
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
            if (!string.IsNullOrEmpty(outputJointName)) {
                jointOffset = new Quaternion(-jointOffset.x, jointOffset.y, jointOffset.z, -jointOffset.w);
                File.AppendAllText(exportFstPath, "jointRotationOffset = " + outputJointName + " = (" + jointOffset.x + ", " +
                                                  jointOffset.y + ", " + jointOffset.z + ", " + jointOffset.w + ")\n");
            }
        }
             
        // open File Explorer to the project directory once finished
        System.Diagnostics.Process.Start("explorer.exe", "/select," + exportFstPath);
        
        return true;
    }
    
    static void SetUserBoneInformation() {
        userBoneInfos.Clear();
        humanoidToUserBoneMappings.Clear();
        userBoneTree = new BoneTreeNode();
        
        // instantiate a game object of the user avatar to traverse the bone tree to gather  
        // bone parents and positions as well as build a bone tree, then destroy it
        UnityEngine.Object avatarResource = AssetDatabase.LoadAssetAtPath(assetPath, typeof(UnityEngine.Object));
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
    }

    static void TraverseUserBoneTree(Transform modelBone) {
        GameObject gameObject = modelBone.gameObject;

        // check if this transform is a node containing mesh, light, or camera instead of a bone
        bool mesh = gameObject.GetComponent<MeshRenderer>() != null || gameObject.GetComponent<SkinnedMeshRenderer>() != null;
        bool light = gameObject.GetComponent<Light>() != null;
        bool camera = gameObject.GetComponent<Camera>() != null;
        
        // if it is in fact a bone, add it to the bone tree as well as user bone infos list with position and parent name
        if (!mesh && !light && !camera) {
            UserBoneInformation userBoneInfo = new UserBoneInformation();
            userBoneInfo.position = modelBone.position; // bone's absolute position
            
            string boneName = modelBone.name;
            if (modelBone.parent == null) {
                // if no parent then this is actual root bone node of the user avatar, so consider it's parent as "root"
                userBoneTree = new BoneTreeNode(boneName); // initialize root of tree
                userBoneInfo.parentName = "root";
                userBoneInfo.boneTreeNode = userBoneTree;
            } else {
                // otherwise add this bone node as a child to it's parent's children list
                string parentName = modelBone.parent.name;
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
    
    static void SetFailedBoneRules() {
        failedBoneRules.Clear();
        
        string hipsUserBone = "";
        string spineUserBone = "";
        string chestUserBone = "";
        string headUserBone = "";
        
        Vector3 hipsPosition = new Vector3();
        
        // iterate over all bone rules in order and add any rules that fail 
        // to the failed bone rules map with appropriate error or warning text
        for (BoneRule boneRule = 0; boneRule < BoneRule.BoneRuleEnd; ++boneRule) { 
            switch (boneRule) {
                case BoneRule.RecommendedUnityVersion:
                    if (Array.IndexOf(RECOMMENDED_UNITY_VERSIONS, Application.unityVersion) == -1) {
                        failedBoneRules.Add(boneRule, "The current version of Unity is not one of the recommended Unity " +
                                                      "versions. If you are using a version of Unity later than 2018.2.12f1, " +
                                                      "it is recommended to apply Enforce T-Pose under the Pose dropdown " +
                                                      "in Humanoid configuration.");
                    }
                    break;
                case BoneRule.SingleRoot:
                    // bone rule fails if the root bone node has more than one child bone
                    if (userBoneTree.children.Count > 1) {
                        failedBoneRules.Add(boneRule, "There is more than one bone at the top level of the selected avatar's " +
                                                      "bone hierarchy. Please ensure all bones for Humanoid mappings are " +
                                                      "under the same bone hierarchy.");
                    }
                    break;
                case BoneRule.NoDuplicateMapping:
                    // bone rule fails if any user bone is mapped to more than one Humanoid bone
                    foreach (var userBoneInfo in userBoneInfos) {
                        string boneName = userBoneInfo.Key;
                        int mappingCount = userBoneInfo.Value.mappingCount;
                        if (mappingCount > 1) {
                            string text = "The " + boneName + " bone is mapped to more than one bone in Humanoid.";
                            if (failedBoneRules.ContainsKey(boneRule)) {
                                failedBoneRules[boneRule] += "\n" + text;
                            } else {
                                failedBoneRules.Add(boneRule, text);
                            }
                        }
                    }
                    break;
                case BoneRule.NoAsymmetricalLegMapping:
                    CheckAsymmetricalMappingRule(boneRule, LEG_MAPPING_SUFFIXES, "leg");
                    break;
                case BoneRule.NoAsymmetricalArmMapping:
                    CheckAsymmetricalMappingRule(boneRule, ARM_MAPPING_SUFFIXES, "arm");
                    break;
                case BoneRule.NoAsymmetricalHandMapping:
                    CheckAsymmetricalMappingRule(boneRule, HAND_MAPPING_SUFFIXES, "hand");
                    break;
                case BoneRule.HipsMapped:
                    hipsUserBone = CheckHumanBoneMappingRule(boneRule, "Hips");
                    break;
                case BoneRule.SpineMapped:
                    spineUserBone = CheckHumanBoneMappingRule(boneRule, "Spine");
                    break;
                case BoneRule.SpineDescendantOfHips:
                    CheckUserBoneDescendantOfHumanRule(boneRule, spineUserBone, "Hips");
                    break;
                case BoneRule.ChestMapped:
                    if (!humanoidToUserBoneMappings.TryGetValue("Chest", out chestUserBone)) {
                        // check to see if there is a child of Spine that we can suggest to be mapped to Chest
                        string spineChild = "";
                        if (!string.IsNullOrEmpty(spineUserBone)) {
                            BoneTreeNode spineTreeNode = userBoneInfos[spineUserBone].boneTreeNode;
                            if (spineTreeNode.children.Count == 1) {
                                spineChild = spineTreeNode.children[0].boneName;
                            }
                        }
                        failedBoneRules.Add(boneRule, "There is no Chest bone mapped in Humanoid for the selected avatar.");
                        // if the only found child of Spine is not yet mapped then add it as a suggestion for Chest mapping 
                        if (!string.IsNullOrEmpty(spineChild) && !userBoneInfos[spineChild].HasHumanMapping()) {
                            failedBoneRules[boneRule] += " It is suggested that you map bone " + spineChild + 
                                                         " to Chest in Humanoid.";
                        }
                    }
                    break;
                case BoneRule.ChestDescendantOfSpine:
                    CheckUserBoneDescendantOfHumanRule(boneRule, chestUserBone, "Spine");
                    break;
                case BoneRule.NeckMapped:
                    CheckHumanBoneMappingRule(boneRule, "Neck");
                    break;
                case BoneRule.HeadMapped:
                    headUserBone = CheckHumanBoneMappingRule(boneRule, "Head");
                    break;
                case BoneRule.HeadDescendantOfChest:
                    CheckUserBoneDescendantOfHumanRule(boneRule, headUserBone, "Chest");
                    break;
                case BoneRule.EyesMapped:
                    bool leftEyeMapped = humanoidToUserBoneMappings.ContainsKey("LeftEye");
                    bool rightEyeMapped = humanoidToUserBoneMappings.ContainsKey("RightEye");
                    if (!leftEyeMapped || !rightEyeMapped) {
                        if (leftEyeMapped && !rightEyeMapped) {
                            failedBoneRules.Add(boneRule, "There is no RightEye bone mapped in Humanoid " + 
                                                          "for the selected avatar.");
                        } else if (!leftEyeMapped && rightEyeMapped) {
                            failedBoneRules.Add(boneRule, "There is no LeftEye bone mapped in Humanoid " + 
                                                          "for the selected avatar.");
                        } else {
                            failedBoneRules.Add(boneRule, "There is no LeftEye or RightEye bone mapped in Humanoid " + 
                                                          "for the selected avatar.");
                        }
                    }
                    break;
                case BoneRule.HipsNotOnGround:
                    // ensure the absolute Y position for the bone mapped to Hips (if its mapped) is at least HIPS_GROUND_MIN_Y
                    if (!string.IsNullOrEmpty(hipsUserBone)) {
                        UserBoneInformation hipsBoneInfo = userBoneInfos[hipsUserBone];
                        hipsPosition = hipsBoneInfo.position;
                        if (hipsPosition.y < HIPS_GROUND_MIN_Y) {
                            failedBoneRules.Add(boneRule, "The bone mapped to Hips in Humanoid (" + hipsUserBone + 
                                                          ") should not be at ground level.");
                        }
                    }
                    break;
                case BoneRule.HipsSpineChestNotCoincident:
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
                            failedBoneRules.Add(boneRule, "The bone mapped to Hips in Humanoid (" + hipsUserBone + 
                                                          "), the bone mapped to Spine in Humanoid (" + spineUserBone + 
                                                          "), and the bone mapped to Chest in Humanoid (" + chestUserBone + 
                                                          ") should not be coincidental.");
                        }
                    }
                    break;
                case BoneRule.TotalBoneCountUnderLimit:
                    int userBoneCount = userBoneInfos.Count;
                    if (userBoneCount > MAXIMUM_USER_BONE_COUNT) {
                        failedBoneRules.Add(boneRule, "The total number of bones in the avatar (" + userBoneCount + 
                                                      ") exceeds the maximum bone limit (" + MAXIMUM_USER_BONE_COUNT + ").");
                    }
                    break;
            }
        }
    }
    
    static string CheckHumanBoneMappingRule(BoneRule boneRule, string humanBoneName) {
        string userBoneName = "";
        // bone rule fails if bone is not mapped in Humanoid
        if (!humanoidToUserBoneMappings.TryGetValue(humanBoneName, out userBoneName)) {
            failedBoneRules.Add(boneRule, "There is no " + humanBoneName + " bone mapped in Humanoid for the selected avatar.");
        }
        return userBoneName;
    }
    
    static void CheckUserBoneDescendantOfHumanRule(BoneRule boneRule, string userBoneName, string descendantOfHumanName) {
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
        
        // bone rule fails if no ancestor of given user bone matched the descendant of name (no early return)
        failedBoneRules.Add(boneRule, "The bone mapped to " + userBoneInfo.humanName + " in Humanoid (" + userBoneName + 
                                      ") is not a child of the bone mapped to " + descendantOfHumanName + " in Humanoid (" + 
                                      descendantOfUserBoneName + ").");
    }
    
    static void CheckAsymmetricalMappingRule(BoneRule boneRule, string[] mappingSuffixes, string appendage) {
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
        // bone rule fails if number of left appendage mappings doesn't match number of right appendage mappings 
        if (leftCount != rightCount) {
            failedBoneRules.Add(boneRule, "The number of bones mapped in Humanoid for the left " + appendage + " (" +
                                          leftCount + ") does not match the number of bones mapped in Humanoid for the right " +
                                          appendage + " (" + rightCount + ").");
        }
    }
    
    static string SetTextureDependencies() {
        string textureWarnings = "";
        dependencyTextures.Clear();
        
        // build the list of all local asset paths for textures that Unity considers dependencies of the model
        // for any textures that have duplicate names, return a string of duplicate name warnings
        string[] dependencies = AssetDatabase.GetDependencies(assetPath);
        foreach (string dependencyPath in dependencies) {
            UnityEngine.Object textureObject = AssetDatabase.LoadAssetAtPath(dependencyPath, typeof(Texture2D));
            if (textureObject != null) {
                string textureName = Path.GetFileName(dependencyPath);
                if (dependencyTextures.ContainsKey(textureName)) {
                    textureWarnings += "There is more than one texture with the name " + textureName + 
									   " referenced in the selected avatar.\n\n";
                } else {
                    dependencyTextures.Add(textureName, dependencyPath);
                }
            }
        }
        
        return textureWarnings;
    }
    
    static bool CopyExternalTextures(string texturesDirectory) {
        // copy the found dependency textures from the local asset folder to the textures folder in the target export project
        foreach (var texture in dependencyTextures) {
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
}

class ExportProjectWindow : EditorWindow {
    const int WINDOW_WIDTH = 500;
    const int WINDOW_HEIGHT = 460;
    const int BUTTON_FONT_SIZE = 16;
    const int LABEL_FONT_SIZE = 16;
    const int TEXT_FIELD_FONT_SIZE = 14;
    const int TEXT_FIELD_HEIGHT = 20;
    const int ERROR_FONT_SIZE = 12;
    const int WARNING_SCROLL_HEIGHT = 170;
    const string EMPTY_ERROR_TEXT = "None\n";
    
    string projectName = "";
    string projectLocation = "";
    string projectDirectory = "";
    string errorText = EMPTY_ERROR_TEXT;
    string warningText = "";
    Vector2 warningScrollPosition = new Vector2(0, 0);
    
    public delegate void OnCloseDelegate(string projectDirectory, string projectName, string warnings);
    OnCloseDelegate onCloseCallback;
    
    public void Init(string initialPath, string warnings, OnCloseDelegate closeCallback) {
        minSize = new Vector2(WINDOW_WIDTH, WINDOW_HEIGHT);
        maxSize = new Vector2(WINDOW_WIDTH, WINDOW_HEIGHT);
        titleContent.text = "Export New Avatar";
        projectLocation = initialPath;
        warningText = warnings;
        onCloseCallback = closeCallback;
        ShowUtility();
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
        warningStyle.normal.textColor = Color.yellow;
     
        GUILayout.Space(10);
        
        // Project name label and input text field
        GUILayout.Label("Export project name:", labelStyle);
        projectName = GUILayout.TextField(projectName, textStyle);
        
        GUILayout.Space(10);
        
        // Project location label and input text field
        GUILayout.Label("Export project location:", labelStyle);
        projectLocation = GUILayout.TextField(projectLocation, textStyle);      
        
        // Browse button to open folder explorer that starts at project location path and then updates project location
        if (GUILayout.Button("Browse", buttonStyle)) {
            string result = EditorUtility.OpenFolderPanel("Select export location", projectLocation, "");
            if (result.Length > 0) { // folder selection not cancelled
                projectLocation = result.Replace('/', '\\');
            }
        }
        
        // Red error label text to display any file-related errors
        GUILayout.Label("Error:", errorStyle);
        GUILayout.Label(errorText, errorStyle);
        
        GUILayout.Space(10);
        
        // Yellow warning label text to display scrollable list of any bone-related warnings 
        GUILayout.Label("Warnings:", warningStyle);
        warningScrollPosition = GUILayout.BeginScrollView(warningScrollPosition, GUILayout.Width(WINDOW_WIDTH), 
                                                          GUILayout.Height(WARNING_SCROLL_HEIGHT));
        GUILayout.Label(warningText, warningStyle);
        GUILayout.EndScrollView();
        
        GUILayout.Space(10);
        
        // Export button which will verify project folder can actually be created 
        // before closing popup window and calling back to initiate the export
        bool export = false;
        if (GUILayout.Button("Export", buttonStyle)) {
            export = true;
            if (!CheckForErrors(true)) {
                Close();
                onCloseCallback(projectDirectory, projectName, warningText);
            }
        }
        
        // Cancel button just closes the popup window without callback
        if (GUILayout.Button("Cancel", buttonStyle)) {
            Close();
        }
        
        // When either text field changes check for any errors if we didn't just check errors from clicking Export above
        if (GUI.changed && !export) {
            CheckForErrors(false);
        }
    }
    
    bool CheckForErrors(bool exporting) {   
        errorText = EMPTY_ERROR_TEXT; // default to None if no errors found
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
        return false;
    }
}
