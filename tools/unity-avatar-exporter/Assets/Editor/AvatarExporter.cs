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
    
    static readonly Dictionary<string, Quaternion> referenceAbsoluteRotations = new Dictionary<string, Quaternion> {
        {"Head", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"Hips", new Quaternion(-3.043941e-10f, -1.573706e-7f, 5.112975e-6f, 1f)},
        {"LeftHandIndex3", new Quaternion(-0.5086057f, 0.4908088f, -0.4912299f, -0.5090388f)},
        {"LeftHandIndex2", new Quaternion(-0.4934928f, 0.5062312f, -0.5064303f, -0.4936835f)},
        {"LeftHandIndex1", new Quaternion(-0.4986293f, 0.5017503f, -0.5013659f, -0.4982448f)},
        {"LeftHandPinky3", new Quaternion(-0.490056f, 0.5143053f, -0.5095307f, -0.4855038f)},
        {"LeftHandPinky2", new Quaternion(-0.5083722f, 0.4954255f, -0.4915887f, -0.5044324f)},
        {"LeftHandPinky1", new Quaternion(-0.5062528f, 0.497324f, -0.4937346f, -0.5025966f)},
        {"LeftHandMiddle3", new Quaternion(-0.4871885f, 0.5123404f, -0.5125002f, -0.4873383f)},
        {"LeftHandMiddle2", new Quaternion(-0.5171652f, 0.4827828f, -0.4822642f, -0.5166069f)},
        {"LeftHandMiddle1", new Quaternion(-0.4955998f, 0.5041052f, -0.5043675f, -0.4958555f)},
        {"LeftHandRing3", new Quaternion(-0.4936301f, 0.5097645f, -0.5061787f, -0.4901562f)},
        {"LeftHandRing2", new Quaternion(-0.5089865f, 0.4943658f, -0.4909532f, -0.5054707f)},
        {"LeftHandRing1", new Quaternion(-0.5020972f, 0.5005084f, -0.4979034f, -0.4994819f)},
        {"LeftHandThumb3", new Quaternion(-0.6617184f, 0.2884935f, -0.3604706f, -0.5907297f)},
        {"LeftHandThumb2", new Quaternion(-0.6935627f, 0.1995147f, -0.2805665f, -0.6328092f)},
        {"LeftHandThumb1", new Quaternion(-0.6663674f, 0.278572f, -0.3507071f, -0.5961183f)},
        {"LeftEye", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"LeftFoot", new Quaternion(0.009215056f, 0.3612514f, 0.9323555f, -0.01121602f)},
        {"LeftHand", new Quaternion(-0.4797408f, 0.5195366f, -0.5279632f, -0.4703038f)},
        {"LeftForeArm", new Quaternion(-0.4594738f, 0.4594729f, -0.5374805f, -0.5374788f)},
        {"LeftLeg", new Quaternion(-0.0005380471f, -0.03154583f, 0.9994993f, 0.002378627f)},
        {"LeftShoulder", new Quaternion(-0.3840606f, 0.525857f, -0.5957767f, -0.47013f)},
        {"LeftToeBase", new Quaternion(-0.0002536641f, 0.7113448f, 0.7027079f, -0.01379319f)},
        {"LeftArm", new Quaternion(-0.4591927f, 0.4591916f, -0.5377204f, -0.5377193f)},
        {"LeftUpLeg", new Quaternion(-0.0006682819f, 0.0006864658f, 0.9999968f, -0.002333928f)},
        {"Neck", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"RightHandIndex3", new Quaternion(0.5083892f, 0.4911618f, -0.4914584f, 0.5086939f)},
        {"RightHandIndex2", new Quaternion(0.4931984f, 0.5065879f, -0.5067145f, 0.4933202f)},
        {"RightHandIndex1", new Quaternion(0.4991491f, 0.5012957f, -0.5008481f, 0.4987026f)},
        {"RightHandPinky3", new Quaternion(0.4890696f, 0.5154139f, -0.5104482f, 0.4843578f)},
        {"RightHandPinky2", new Quaternion(0.5084175f, 0.495413f, -0.4915423f, 0.5044444f)},
        {"RightHandPinky1", new Quaternion(0.5069782f, 0.4965974f, -0.4930001f, 0.5033045f)},
        {"RightHandMiddle3", new Quaternion(0.4867662f, 0.5129694f, -0.5128888f, 0.4866894f)},
        {"RightHandMiddle2", new Quaternion(0.5167004f, 0.4833596f, -0.4827653f, 0.5160643f)},
        {"RightHandMiddle1", new Quaternion(0.4965845f, 0.5031784f, -0.5033959f, 0.4967981f)},
        {"RightHandRing3", new Quaternion(0.4933217f, 0.5102056f, -0.5064691f, 0.4897075f)},
        {"RightHandRing2", new Quaternion(0.5085972f, 0.494844f, -0.4913519f, 0.505007f)},
        {"RightHandRing1", new Quaternion(0.502959f, 0.4996676f, -0.4970418f, 0.5003144f)},
        {"RightHandThumb3", new Quaternion(0.6611374f, 0.2896575f, -0.3616535f, 0.5900872f)},
        {"RightHandThumb2", new Quaternion(0.6937408f, 0.1986776f, -0.279922f, 0.6331626f)},
        {"RightHandThumb1", new Quaternion(0.6664271f, 0.2783172f, -0.3505667f, 0.596253f)},
        {"RightEye", new Quaternion(-2.509889e-9f, -3.379446e-12f, 2.306033e-13f, 1f)},
        {"RightFoot", new Quaternion(-0.009482829f, 0.3612484f, 0.9323512f, 0.01144584f)},
        {"RightHand", new Quaternion(0.4797273f, 0.5195542f, -0.5279628f, 0.4702987f)},
        {"RightForeArm", new Quaternion(0.4594217f, 0.4594215f, -0.5375242f, 0.5375237f)},
        {"RightLeg", new Quaternion(0.0005446263f, -0.03177159f, 0.9994922f, -0.002395923f)},
        {"RightShoulder", new Quaternion(0.3841222f, 0.5257177f, -0.5957286f, 0.4702966f)},
        {"RightToeBase", new Quaternion(0.0001034f, 0.7113398f, 0.7027067f, 0.01411251f)},
        {"RightArm", new Quaternion(0.4591419f, 0.4591423f, -0.537763f, 0.5377624f)},
        {"RightUpLeg", new Quaternion(0.0006750703f, 0.0008973633f, 0.9999966f, 0.002352045f)},
        {"Spine", new Quaternion(-0.05427956f, 1.508558e-7f, -2.775203e-6f, 0.9985258f)},
        {"Spine1", new Quaternion(-0.0824653f, 1.25274e-7f, -6.75759e-6f, 0.996594f)},
        {"Spine2", new Quaternion(-0.0824653f, 1.25274e-7f, -6.75759e-6f, 0.996594f)},
    };

    static Dictionary<string, string> userBoneToHumanoidMappings = new Dictionary<string, string>();
    static Dictionary<string, string> userParentNames = new Dictionary<string, string>();
    static Dictionary<string, Quaternion> userAbsoluteRotations = new Dictionary<string, Quaternion>();
    
    static string assetPath = "";
    static string assetName = "";
    static HumanDescription humanDescription;
 
    [MenuItem("High Fidelity/Export New Avatar")]
    static void ExportNewAvatar() {
        ExportSelectedAvatar(false);
    }

    [MenuItem("High Fidelity/Update Existing Avatar")]
    static void UpdateAvatar() {
        ExportSelectedAvatar(true);
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
            EditorUtility.DisplayDialog("Error", "Please set model's Animation Type to Humanoid in the Rig section of it's Inspector window.", "Ok");
            return;
        }
                
        humanDescription = modelImporter.humanDescription;   
        if (!SetJointMappingsAndParentNames()) {
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
                        File.Copy(exportModelPath, assetPath, true);
                        AssetDatabase.ImportAsset(assetPath);
                        
                        // set model to Humanoid animation type and force another refresh on it to process Humanoid
                        modelImporter = ModelImporter.GetAtPath(assetPath) as ModelImporter;
                        modelImporter.animationType = ModelImporterAnimationType.Human;
                        EditorUtility.SetDirty(modelImporter);
                        modelImporter.SaveAndReimport();
                        humanDescription = modelImporter.humanDescription;   
                        
                        // redo joint mappings and parent names due to the fbx change
                        SetJointMappingsAndParentNames();
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
            WriteFST(exportFstPath, projectName);
        } else { // Export New Avatar menu option
            // create High Fidelity Projects folder in user documents folder if it doesn't exist
            if (!Directory.Exists(hifiFolder)) {    
                Directory.CreateDirectory(hifiFolder);
            }
            
            // open a popup window to enter new export project name and project location
            ExportProjectWindow window = ScriptableObject.CreateInstance<ExportProjectWindow>();
            window.Init(hifiFolder, OnExportProjectWindowClose);
        }
    }
    
    static void OnExportProjectWindowClose(string projectDirectory, string projectName) {
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
        WriteFST(exportFstPath, projectName);
        
        // remove any double slashes in texture directory path and warn user to copy external textures over
        texturesDirectory = texturesDirectory.Replace("\\\\", "\\");
        EditorUtility.DisplayDialog("Warning", "If you are using any external textures with your model, " +
                                               "please copy those textures to " + texturesDirectory, "Ok");
    }
    
    static bool SetJointMappingsAndParentNames() {
        userParentNames.Clear();
        userBoneToHumanoidMappings.Clear();
        
        // instantiate a game object of the user avatar to save out bone parents then destroy it
        UnityEngine.Object avatarResource = AssetDatabase.LoadAssetAtPath(assetPath, typeof(UnityEngine.Object));
        GameObject assetGameObject = (GameObject)Instantiate(avatarResource);
        SetParentNames(assetGameObject.transform, userParentNames);
        DestroyImmediate(assetGameObject);
        
        // store joint mappings only for joints that exist in hifi and verify missing required joints
        HumanBone[] boneMap = humanDescription.human;
        string chestUserBone = "";
        string neckUserBone = "";
        foreach (HumanBone bone in boneMap) {
            string humanName = bone.humanName;
            string boneName = bone.boneName;
            string hifiJointName;
            if (HUMANOID_TO_HIFI_JOINT_NAME.TryGetValue(humanName, out hifiJointName)) {
                userBoneToHumanoidMappings.Add(boneName, humanName);
                if (humanName == "Chest") {
                    chestUserBone = boneName;
                } else if (humanName == "Neck") {
                    neckUserBone = boneName;
                }
            }

        }
        if (!userBoneToHumanoidMappings.ContainsValue("Hips")) {
            EditorUtility.DisplayDialog("Error", "There is no Hips bone in selected avatar", "Ok");
            return false;
        }
        if (!userBoneToHumanoidMappings.ContainsValue("Spine")) {
            EditorUtility.DisplayDialog("Error", "There is no Spine bone in selected avatar", "Ok");
            return false;
        }
        if (!userBoneToHumanoidMappings.ContainsValue("Chest")) {
            // check to see if there is a child of Spine that could be mapped to Chest
            string spineChild = "";
            foreach (var parentRelation in userParentNames) {
                string humanName;
                if (userBoneToHumanoidMappings.TryGetValue(parentRelation.Value, out humanName) && humanName == "Spine") {
                    if (spineChild == "") {
                        spineChild = parentRelation.Key;
                    } else {
                        // found more than one Spine child so we can't choose one to remap
                        spineChild = "";
                        break;
                    }
                }
            }
            if (spineChild != "" && !userBoneToHumanoidMappings.ContainsKey(spineChild)) {
                // use child of Spine as Chest
                userBoneToHumanoidMappings.Add(spineChild, "Chest");
                chestUserBone = spineChild;
            } else {
                EditorUtility.DisplayDialog("Error", "There is no Chest bone in selected avatar", "Ok");
                return false;
            }
        }
        if (!userBoneToHumanoidMappings.ContainsValue("UpperChest")) {
            //if parent of Neck is not Chest then map the parent to UpperChest
            if (neckUserBone != "") {
                string neckParentUserBone, neckParentHuman;
                userParentNames.TryGetValue(neckUserBone, out neckParentUserBone);
                userBoneToHumanoidMappings.TryGetValue(neckParentUserBone, out neckParentHuman);
                if (neckParentHuman != "Chest" && !userBoneToHumanoidMappings.ContainsKey(neckParentUserBone)) {
                    userBoneToHumanoidMappings.Add(neckParentUserBone, "UpperChest");
                }
            }
            // if there is still no UpperChest bone but there is a Chest bone then we remap Chest to UpperChest 
            if (!userBoneToHumanoidMappings.ContainsValue("UpperChest") && chestUserBone != "") {       
                userBoneToHumanoidMappings[chestUserBone] = "UpperChest";
            }
        }
        
        return true;
    }
    
    static void WriteFST(string exportFstPath, string projectName) {
        userAbsoluteRotations.Clear();
        
        // write out core fields to top of fst file
        try {
            File.WriteAllText(exportFstPath, "name = " + projectName + "\ntype = body+head\nscale = 1\nfilename = " + 
                              assetName + ".fbx\n" + "texdir = textures\n");
        } catch { 
            EditorUtility.DisplayDialog("Error", "Failed to write file " + exportFstPath + 
                                        ". Please check the location and try again.", "Ok");
            return;
        }
        
        // write out joint mappings to fst file
        foreach (var jointMapping in userBoneToHumanoidMappings) {
            string hifiJointName = HUMANOID_TO_HIFI_JOINT_NAME[jointMapping.Value];
            File.AppendAllText(exportFstPath, "jointMap = " + hifiJointName + " = " + jointMapping.Key + "\n");
        }
        
        // calculate and write out joint rotation offsets to fst file
        SkeletonBone[] skeletonMap = humanDescription.skeleton;
        foreach (SkeletonBone userBone in skeletonMap) {
            string userBoneName = userBone.name;
            Quaternion userBoneRotation = userBone.rotation;            
            
            string parentName;
            userParentNames.TryGetValue(userBoneName, out parentName);
            if (parentName == "root") {
                // if the parent is root then use bone's rotation
                userAbsoluteRotations.Add(userBoneName, userBoneRotation);
            } else {
                // otherwise multiply bone's rotation by parent bone's absolute rotation
                userAbsoluteRotations.Add(userBoneName, userAbsoluteRotations[parentName] * userBoneRotation);
            }
            
            // generate joint rotation offsets for both humanoid-mapped bones as well as extra unmapped bones in user avatar
            Quaternion jointOffset = new Quaternion();
            string humanName, outputJointName = "";
            if (userBoneToHumanoidMappings.TryGetValue(userBoneName, out humanName)) {
                outputJointName = HUMANOID_TO_HIFI_JOINT_NAME[humanName];
                Quaternion rotation = referenceAbsoluteRotations[outputJointName];
                jointOffset = Quaternion.Inverse(userAbsoluteRotations[userBoneName]) * rotation;
            } else if (userAbsoluteRotations.ContainsKey(userBoneName)) {
                outputJointName = userBoneName;
                string lastRequiredParent = FindLastRequiredParentBone(userBoneName);
                if (lastRequiredParent == "root") {
                    jointOffset = Quaternion.Inverse(userAbsoluteRotations[userBoneName]);
                } else {
                    // take the previous offset and multiply it by the current local when we have an extra joint
                    string lastRequiredParentHifiName = HUMANOID_TO_HIFI_JOINT_NAME[userBoneToHumanoidMappings[lastRequiredParent]];
                    Quaternion lastRequiredParentRotation = referenceAbsoluteRotations[lastRequiredParentHifiName];
                    jointOffset = Quaternion.Inverse(userAbsoluteRotations[userBoneName]) * lastRequiredParentRotation;
                }
            }
            
            // swap from left-handed (Unity) to right-handed (HiFi) coordinates and write out joint rotation offset to fst
            if (outputJointName != "") {
                jointOffset = new Quaternion(-jointOffset.x, jointOffset.y, jointOffset.z, -jointOffset.w);
                File.AppendAllText(exportFstPath, "jointRotationOffset = " + outputJointName + " = (" + jointOffset.x + ", " +
                                                  jointOffset.y + ", " + jointOffset.z + ", " + jointOffset.w + ")\n");
            }
        }
             
        // open File Explorer to the project directory once finished
        System.Diagnostics.Process.Start("explorer.exe", "/select," + exportFstPath);
    }

    static void SetParentNames(Transform modelBone, Dictionary<string, string> parentNames) {
        for (int i = 0; i < modelBone.childCount; i++) {
            SetParentNames(modelBone.GetChild(i), parentNames);
        }
        if (modelBone.parent != null) {
            parentNames.Add(modelBone.name, modelBone.parent.name);
        } else {
            parentNames.Add(modelBone.name, "root");
        }
    }
    
    static string FindLastRequiredParentBone(string currentBone) {
        string result = currentBone;
        while (result != "root" && !userBoneToHumanoidMappings.ContainsKey(result)) {
            result = userParentNames[result];            
        }
        return result;
    }
}

class ExportProjectWindow : EditorWindow {
    const int MIN_WIDTH = 450;
    const int MIN_HEIGHT = 250;
    const int BUTTON_FONT_SIZE = 16;
    const int LABEL_FONT_SIZE = 16;
    const int TEXT_FIELD_FONT_SIZE = 14;
    const int TEXT_FIELD_HEIGHT = 20;
    const int ERROR_FONT_SIZE = 12;
    
    string projectName = "";
    string projectLocation = "";
    string projectDirectory = "";
    string errorLabel = "\n";
    
    public delegate void OnCloseDelegate(string projectDirectory, string projectName);
    OnCloseDelegate onCloseCallback;
    
    public void Init(string initialPath, OnCloseDelegate closeCallback) {
        minSize = new Vector2(MIN_WIDTH, MIN_HEIGHT);
        titleContent.text = "Export New Avatar";
        projectLocation = initialPath;
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
        
        // Red error label text to display any issues under text fields and Browse button
        GUILayout.Label(errorLabel, errorStyle);
        
        GUILayout.Space(20);
        
        // Export button which will verify project folder can actually be created 
        // before closing popup window and calling back to initiate the export
        bool export = false;
        if (GUILayout.Button("Export", buttonStyle)) {
            export = true;
            if (!CheckForErrors(true)) {
                Close();
                onCloseCallback(projectDirectory, projectName);
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
        errorLabel = "\n"; // default to no error
        projectDirectory = projectLocation + "\\" + projectName + "\\";
        if (projectName.Length > 0) {
            // new project must have a unique folder name since the folder will be created for it
            if (Directory.Exists(projectDirectory)) {
                errorLabel = "A folder with the name " + projectName + 
                             " already exists at that location.\nPlease choose a different project name or location.";
                return true;
            }
        }
        if (projectLocation.Length > 0) {
            // before clicking Export we can verify that the project location at least starts with a drive
            if (!Char.IsLetter(projectLocation[0]) || projectLocation.Length == 1 || projectLocation[1] != ':') {
                errorLabel = "Project location is invalid. Please choose a different project location.\n";
                return true;
            }
        }
        if (exporting) {
            // when exporting, project name and location must both be defined, and project location must
            // be valid and accessible (we attempt to create the project folder at this time to verify this)
            if (projectName.Length == 0) {
                errorLabel = "Please define a project name.\n";
                return true;
            } else if (projectLocation.Length == 0) {
                errorLabel = "Please define a project location.\n";
                return true;
            } else {
                try {
                    Directory.CreateDirectory(projectDirectory);
                } catch {
                    errorLabel = "Project location is invalid. Please choose a different project location.\n";
                    return true;
                }
            }
        } 
        return false;
    }
}
