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

public class AvatarExporter : MonoBehaviour {   
    public static Dictionary<string, string> HUMANOID_TO_HIFI_JOINT_NAME = new Dictionary<string, string> {
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
    
    public static Dictionary<string, Quaternion> referenceAbsoluteRotations = new Dictionary<string, Quaternion> {
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
        {"LeftHandThumb3", new Quaternion(-0.7228092f, 0.2988393f, -0.4472938f, -0.4337862f)},
        {"LeftHandThumb2", new Quaternion(-0.7554525f, 0.2018595f, -0.3871402f, -0.4885356f)},
        {"LeftHandThumb1", new Quaternion(-0.7276843f, 0.2878546f, -0.439926f, -0.4405459f)},
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
        {"RightHandThumb3", new Quaternion(0.7221864f, 0.3001843f, -0.4482129f, 0.4329457f)},
        {"RightHandThumb2", new Quaternion(0.755621f, 0.20102f, -0.386691f, 0.4889769f)},
        {"RightHandThumb1", new Quaternion(0.7277303f, 0.2876409f, -0.4398623f, 0.4406733f)},
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

    public static Dictionary<string, string> userBoneToHumanoidMappings = new Dictionary<string, string>();
    public static Dictionary<string, string> userParentNames = new Dictionary<string, string>();
    public static Dictionary<string, Quaternion> userAbsoluteRotations = new Dictionary<string, Quaternion>();
 
    [MenuItem("High Fidelity/Export New Avatar")]
    public static void ExportNewAvatar() {
        ExportSelectedAvatar(false);
    }

    [MenuItem("High Fidelity/Update Avatar")]
    public static void UpdateAvatar() {
        ExportSelectedAvatar(true);
    }

    public static void ExportSelectedAvatar(bool updateAvatar) {     
        string[] guids = Selection.assetGUIDs;
        if (guids.Length != 1) {
            if (guids.Length == 0) {
                EditorUtility.DisplayDialog("Error", "Please select an asset to export", "Ok");
            } else {
                EditorUtility.DisplayDialog("Error", "Please select a single asset to export", "Ok");
            }
            return;
        }
        string assetPath = AssetDatabase.GUIDToAssetPath(Selection.assetGUIDs[0]);
        string assetName = Path.GetFileNameWithoutExtension(assetPath);
        string assetDirectory = Path.GetDirectoryName(assetPath);
        if (assetDirectory != "Assets/Resources") {
            EditorUtility.DisplayDialog("Error", "Please place asset in the Assets/Resources folder", "Ok");
            return;
        }       
        ModelImporter importer = ModelImporter.GetAtPath(assetPath) as ModelImporter;
        if (assetPath.LastIndexOf(".fbx") == -1 || importer == null) {
            EditorUtility.DisplayDialog("Error", "Please select an fbx model asset to export", "Ok");
            return;
        }
        if (importer.animationType != ModelImporterAnimationType.Human) {
            EditorUtility.DisplayDialog("Error", "Please set model's Animation Type to Humanoid", "Ok");
            return;
        }
        
        userBoneToHumanoidMappings.Clear();
        userParentNames.Clear();
        userAbsoluteRotations.Clear();
        
        // instantiate a game object of the user avatar to save out bone parents then destroy it
        UnityEngine.Object avatarResource = Resources.Load(assetName);
        if (avatarResource) {
             GameObject assetGameObject = (GameObject)Instantiate(avatarResource);
             SetParentNames(assetGameObject.transform, userParentNames);
             DestroyImmediate(assetGameObject);
        }
        
        // store joint mappings only for joints that exist in hifi and verify missing joints
        HumanDescription humanDescription = importer.humanDescription;
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
            return;
        }
        if (!userBoneToHumanoidMappings.ContainsValue("Spine")) {
            EditorUtility.DisplayDialog("Error", "There is no Spine bone in selected avatar", "Ok");
            return;
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
                return;
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
    
        bool copyModelToExport = false;
        string exportFstPath, exportModelPath;
        string documentsFolder = System.Environment.GetFolderPath(System.Environment.SpecialFolder.MyDocuments);
        if (updateAvatar) {
            // open file explorer defaulting to user documents folder to select target fst to update
            exportFstPath = EditorUtility.OpenFilePanel("Select fst to update", documentsFolder, "fst");
            if (exportFstPath.Length == 0) { // file selection cancelled
                return;
            }
            exportModelPath = Path.GetDirectoryName(exportFstPath) + "/" + assetName + ".fbx";
            
            if (File.Exists(exportModelPath)) { 
                // if the fbx in Unity Assets/Resources is newer than the fbx in the 
                // target export folder or vice-versa then ask to copy fbx over
                DateTime assetModelWriteTime = File.GetLastWriteTime(assetPath);
                DateTime targetModelWriteTime = File.GetLastWriteTime(exportModelPath);
                if (assetModelWriteTime > targetModelWriteTime) {
                    copyModelToExport = EditorUtility.DisplayDialog("Error", "The " + assetName + 
                                        ".fbx model in the Unity Assets/Resources folder is newer than the " + exportModelPath +
                                        " model. Do you want to copy the newer .fbx model over?" , "Yes", "No");
                } else if (assetModelWriteTime < targetModelWriteTime) {
                    bool copyModelToUnity = EditorUtility.DisplayDialog("Error", "The " + exportModelPath + 
                                            " model is newer than the " + assetName + 
                                            ".fbx model in the Unity Assets/Resources folder. Do you want to copy the newer .fbx model over?",
                                            "Yes", "No");
                    if (copyModelToUnity) {
                        File.Delete(assetPath);
                        File.Copy(exportModelPath, assetPath);
                    }
                }
            } else {
                // if no matching fbx exists in the target export folder then ask to copy fbx over
                copyModelToExport = EditorUtility.DisplayDialog("Error", "There is no existing " + exportModelPath + 
                                    " model. Do you want to copy over the " + assetName + 
                                    ".fbx model from the Unity Assets/Resources folder?" , "Yes", "No");
            }   
        } else {
            // open folder explorer defaulting to user documents folder to select target folder to export fst and fbx to
            if (!SelectExportFolder(assetName, documentsFolder, out exportFstPath, out exportModelPath)) {
                return;
            }
            copyModelToExport = true;
        }
        
        // delete any existing fbx since we would have agreed to overwrite it, and copy asset fbx over
        if (copyModelToExport) {
            if (File.Exists(exportModelPath)) {
                File.Delete(exportModelPath);
            }
            File.Copy(assetPath, exportModelPath);
        }
        
        // delete any existing fst since we agreed to overwrite it or are updating it
        // TODO: should updating fst only rewrite joint mappings and joint rotation offsets?
        if (File.Exists(exportFstPath)) {
            File.Delete(exportFstPath);
        }
        
        // write out core fields to top of fst file
        File.WriteAllText(exportFstPath, "name = " + assetName + "\ntype = body+head\nscale = 1\nfilename = " + 
                          assetName + ".fbx\n" + "texdir = textures\n");
        
        // write out joint mappings to fst file
        foreach (var jointMapping in userBoneToHumanoidMappings) {
            string hifiJointName = HUMANOID_TO_HIFI_JOINT_NAME[jointMapping.Value];
            File.AppendAllText(exportFstPath, "jointMap = " + hifiJointName + " = " + jointMapping.Key + "\n");
        }
        
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
                string lastRequiredParent = FindLastRequiredParentBone(userBoneName);
                if (lastRequiredParent != "root") {
                    // take the previous offset and multiply it by the current local when we have an extra joint
                    outputJointName = userBoneName;
                    string lastRequiredParentHifiName = HUMANOID_TO_HIFI_JOINT_NAME[userBoneToHumanoidMappings[lastRequiredParent]];
                    Quaternion lastRequiredParentRotation = referenceAbsoluteRotations[lastRequiredParentHifiName];
                    jointOffset = Quaternion.Inverse(userAbsoluteRotations[userBoneName]) * lastRequiredParentRotation;
                }
            }
            
            // swap from left-handed (Unity) to right-handed (HiFi) coordinate system and write out joint rotation offset to fst
            if (outputJointName != "") {
                jointOffset = new Quaternion(-jointOffset.x, jointOffset.y, jointOffset.z, -jointOffset.w);
                File.AppendAllText(exportFstPath, "jointRotationOffset = " + outputJointName + " = (" + jointOffset.x + ", " +
                                                  jointOffset.y + ", " + jointOffset.z + ", " + jointOffset.w + ")\n");
            }
        }
    }
    
    public static bool SelectExportFolder(string assetName, string initialPath, out string fstPath, out string modelPath) {
        string selectedPath = EditorUtility.OpenFolderPanel("Select export location", initialPath, "");
        if (selectedPath.Length == 0) { // folder selection cancelled
            fstPath = "";
            modelPath = "";
            return false;
        }
        fstPath = selectedPath + "/" + assetName + ".fst";
        modelPath = selectedPath + "/" + assetName + ".fbx";
        bool fstExists = File.Exists(fstPath);
        bool modelExists = File.Exists(modelPath);
        if (fstExists || modelExists) {
            string overwriteMessage;
            if (fstExists && modelExists) {
                overwriteMessage = assetName + ".fst and " + assetName + 
                                   ".fbx already exist here, would you like to overwrite them?";
            } else if (fstExists) {
                overwriteMessage = assetName + ".fst already exists here, would you like to overwrite it?";
            } else {
                overwriteMessage = assetName + ".fbx already exists here, would you like to overwrite it?";
            }
            bool overwrite = EditorUtility.DisplayDialog("Error", overwriteMessage, "Yes", "No");
            if (!overwrite) {
                return SelectExportFolder(assetName, selectedPath, out fstPath, out modelPath);
            }
        }
        return true;
    }
    
    public static void SetParentNames(Transform modelBone, Dictionary<string, string> parentNames) {
        for (int i = 0; i < modelBone.childCount; i++) {
            SetParentNames(modelBone.GetChild(i), parentNames);
        }
        if (modelBone.parent != null) {
            parentNames.Add(modelBone.name, modelBone.parent.name);
        } else {
            parentNames.Add(modelBone.name, "root");
        }
    }
    
    public static string FindLastRequiredParentBone(string currentBone) {
        string result = currentBone;
        while (result != "root" && !userBoneToHumanoidMappings.ContainsKey(result)) {
            result = userParentNames[result];            
        }
        return result;
    }
}
