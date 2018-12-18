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
    public static Dictionary<string, string> UNITY_TO_HIFI_JOINT_NAME = new Dictionary<string, string> {
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
    
    public static string exportedPath = String.Empty;
 
    [MenuItem("High Fidelity/Export New Avatar")]
    public static void ExportNewAvatar() {
        ExportSelectedAvatar(false);
    }
    
    [MenuItem("High Fidelity/Export New Avatar", true)]
    private static bool ExportNewAvatarValidator() {
        // only enable Export New Avatar option if we have an asset selected
        string[] guids = Selection.assetGUIDs;
        return guids.Length > 0;
    }
     
    [MenuItem("High Fidelity/Update Avatar")]
    public static void UpdateAvatar() {
        ExportSelectedAvatar(true);
    }

    [MenuItem("High Fidelity/Update Avatar", true)]
    private static bool UpdateAvatarValidation() {
        // only enable Update Avatar option if the selected avatar is the last one that was exported
        if (exportedPath != String.Empty) {
            string[] guids = Selection.assetGUIDs;
            if (guids.Length > 0) {
                string selectedAssetPath = AssetDatabase.GUIDToAssetPath(guids[0]);
                string selectedAsset = Path.GetFileNameWithoutExtension(selectedAssetPath);
                string exportedAsset = Path.GetFileNameWithoutExtension(exportedPath);
                return exportedAsset == selectedAsset;
            }
        }
        return false;
    }
    
    public static void ExportSelectedAvatar(bool usePreviousPath) {     
        string assetPath = AssetDatabase.GUIDToAssetPath(Selection.assetGUIDs[0]);
        if (assetPath.LastIndexOf(".fbx") == -1) {
            EditorUtility.DisplayDialog("Error", "Please select an fbx avatar to export", "Ok");
            return;
        }
        ModelImporter importer = ModelImporter.GetAtPath(assetPath) as ModelImporter;
        if (importer == null) {
            EditorUtility.DisplayDialog("Error", "Please select a model", "Ok");
            return;
        }
        if (importer.animationType != ModelImporterAnimationType.Human) {
            EditorUtility.DisplayDialog("Error", "Please set model's Animation Type to Humanoid", "Ok");
            return;
        }
        
        // store joint mappings only for joints that exist in hifi and verify missing joints
        HumanDescription humanDescription = importer.humanDescription;
        HumanBone[] boneMap = humanDescription.human;
        Dictionary<string, string> jointMappings = new Dictionary<string, string>();
        foreach (HumanBone bone in boneMap) {
            string humanBone = bone.humanName;
            string hifiJointName;
            if (UNITY_TO_HIFI_JOINT_NAME.TryGetValue(humanBone, out hifiJointName)) {
                jointMappings.Add(hifiJointName, bone.boneName);
            }
        }
        if (!jointMappings.ContainsKey("Hips")) {
            EditorUtility.DisplayDialog("Error", "There is no Hips bone in selected avatar", "Ok");
            return;
        }
        if (!jointMappings.ContainsKey("Spine")) {
            EditorUtility.DisplayDialog("Error", "There is no Spine bone in selected avatar", "Ok");
            return;
        }
        if (!jointMappings.ContainsKey("Spine1")) {
            EditorUtility.DisplayDialog("Error", "There is no Chest bone in selected avatar", "Ok");
            return;
        }
        if (!jointMappings.ContainsKey("Spine2")) {
            // if there is no UpperChest (Spine2) bone then we remap Chest (Spine1) to Spine2 in hifi and skip Spine1
            jointMappings["Spine2"] = jointMappings["Spine1"];
            jointMappings.Remove("Spine1");
        }
    
        // open folder explorer defaulting to user documents folder to select target path if exporting new avatar,
        // otherwise use previously exported path if updating avatar
        string directoryPath;
        string assetName = Path.GetFileNameWithoutExtension(assetPath);
        if (!usePreviousPath || exportedPath == String.Empty) {
            string documentsFolder = System.Environment.GetFolderPath(System.Environment.SpecialFolder.MyDocuments);
            if (!SelectExportFolder(assetName, documentsFolder, out directoryPath)) {
                return;
            }
        } else {
            directoryPath = Path.GetDirectoryName(exportedPath) + "/";
        }
        Directory.CreateDirectory(directoryPath);
        
        // delete any existing fst since we agreed to overwrite it
        string fstPath = directoryPath + assetName + ".fst";
        if (File.Exists(fstPath)) {
            File.Delete(fstPath);
        }
        
        // write out core fields to top of fst file
        File.WriteAllText(fstPath, "name = " + assetName + "\ntype = body+head\nscale = 1\nfilename = " + 
                          assetName + ".fbx\n" + "texdir = textures\n");
        
        // write out joint mappings to fst file
        foreach (var jointMapping in jointMappings) {
            File.AppendAllText(fstPath, "jointMap = " + jointMapping.Key + " = " + jointMapping.Value + "\n");
        }

        // delete any existing fbx since we agreed to overwrite it, and copy fbx over
        string targetAssetPath = directoryPath + assetName + ".fbx";
        if (File.Exists(targetAssetPath)) {
            File.Delete(targetAssetPath);
        }
        File.Copy(assetPath, targetAssetPath);
        
        exportedPath = targetAssetPath;
    }
    
    public static bool SelectExportFolder(string assetName, string initialPath, out string directoryPath) {
        string selectedPath = EditorUtility.OpenFolderPanel("Select export location", initialPath, "");
        if (selectedPath.Length == 0) { // folder selection cancelled
            directoryPath = "";
            return false;
        }
        directoryPath = selectedPath + "/" + assetName + "/";
        if (Directory.Exists(directoryPath)) {
            bool overwrite = EditorUtility.DisplayDialog("Error", "Directory " + assetName + 
                                                         " already exists here, would you like to overwrite it?", "Yes", "No");
            if (!overwrite) {
                SelectExportFolder(assetName, selectedPath, out directoryPath);
            }
        }
        return true;
    }
}
