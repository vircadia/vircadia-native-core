//
//  FST.cpp
//
//  Created by Ryan Huffman on 12/11/15.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "FST.h"

#include <QDir>
#include <QFileInfo>
#include <hfm/HFM.h>

constexpr float DEFAULT_SCALE { 1.0f };

FST::FST(QString fstPath, QMultiHash<QString, QVariant> data) : _fstPath(std::move(fstPath)) {

    auto setValueFromFSTData = [&data] (const QString& propertyID, auto &targetProperty) mutable {
        if (data.contains(propertyID)) {
            targetProperty = data.value(propertyID).toString();
            data.remove(propertyID);
        }
    };
    setValueFromFSTData(NAME_FIELD, _name);
    setValueFromFSTData(FILENAME_FIELD, _modelPath);
    setValueFromFSTData(MARKETPLACE_ID_FIELD, _marketplaceID);

    if (data.contains(SCRIPT_FIELD)) {
        QVariantList scripts = data.values(SCRIPT_FIELD);
        for (const auto& script : scripts) {
            _scriptPaths.push_back(script.toString());
        }
        data.remove(SCRIPT_FIELD);
    }

    _other = data;
}

FST* FST::createFSTFromModel(const QString& fstPath, const QString& modelFilePath, const hfm::Model& hfmModel) {
    QVariantHash mapping;

    // mixamo files - in the event that a mixamo file was edited by some other tool, it's likely the applicationName will
    // be rewritten, so we detect the existence of several different blendshapes which indicate we're likely a mixamo file
    bool likelyMixamoFile = hfmModel.applicationName == "mixamo.com" ||
        (hfmModel.blendshapeChannelNames.contains("BrowsDown_Right") &&
            hfmModel.blendshapeChannelNames.contains("MouthOpen") &&
            hfmModel.blendshapeChannelNames.contains("Blink_Left") &&
            hfmModel.blendshapeChannelNames.contains("Blink_Right") &&
            hfmModel.blendshapeChannelNames.contains("Squint_Right"));

    mapping.insert(NAME_FIELD, QFileInfo(fstPath).baseName());
    mapping.insert(FILENAME_FIELD, QFileInfo(modelFilePath).fileName());
    mapping.insert(TEXDIR_FIELD, "textures");

    // mixamo/autodesk defaults
    mapping.insert(SCALE_FIELD, DEFAULT_SCALE);
    QVariantHash joints = mapping.value(JOINT_FIELD).toHash();
        joints.insert("jointEyeLeft", hfmModel.jointIndices.contains("jointEyeLeft") ? "jointEyeLeft" :
            (hfmModel.jointIndices.contains("EyeLeft") ? "EyeLeft" : "LeftEye"));

    joints.insert("jointEyeRight", hfmModel.jointIndices.contains("jointEyeRight") ? "jointEyeRight" :
            hfmModel.jointIndices.contains("EyeRight") ? "EyeRight" : "RightEye");

    joints.insert("jointNeck", hfmModel.jointIndices.contains("jointNeck") ? "jointNeck" : "Neck");
    joints.insert("jointRoot", "Hips");
    joints.insert("jointLean", "Spine");
    joints.insert("jointLeftHand", "LeftHand");
    joints.insert("jointRightHand", "RightHand");

    const char* topName = likelyMixamoFile ? "HeadTop_End" : "HeadEnd";
    joints.insert("jointHead", hfmModel.jointIndices.contains(topName) ? topName : "Head");

    mapping.insert(JOINT_FIELD, joints);

    QVariantHash jointIndices;
    for (int i = 0; i < hfmModel.joints.size(); i++) {
        jointIndices.insert(hfmModel.joints.at(i).name, QString::number(i));
    }
    mapping.insert(JOINT_INDEX_FIELD, jointIndices);


    // If there are no blendshape mappings, and we detect that this is likely a mixamo file,
    // then we can add the default mixamo to blendshape mappings.
    if (likelyMixamoFile) {
        QVariantHash blendshapes;
        blendshapes.insert("BrowsD_L", QVariantList() << "BrowsDown_Left" << 1.0);
        blendshapes.insert("BrowsD_R", QVariantList() << "BrowsDown_Right" << 1.0);
        blendshapes.insert("BrowsU_C", QVariantList() << "BrowsUp_Left" << 1.0);
        blendshapes.insert("BrowsU_C", QVariantList() << "BrowsUp_Right" << 1.0);
        blendshapes.insert("BrowsU_L", QVariantList() << "BrowsUp_Left" << 1.0);
        blendshapes.insert("BrowsU_R", QVariantList() << "BrowsUp_Right" << 1.0);
        blendshapes.insert("ChinLowerRaise", QVariantList() << "Jaw_Up" << 1.0);
        blendshapes.insert("ChinUpperRaise", QVariantList() << "UpperLipUp_Left" << 0.5);
        blendshapes.insert("ChinUpperRaise", QVariantList() << "UpperLipUp_Right" << 0.5);
        blendshapes.insert("EyeBlink_L", QVariantList() << "Blink_Left" << 1.0);
        blendshapes.insert("EyeBlink_R", QVariantList() << "Blink_Right" << 1.0);
        blendshapes.insert("EyeOpen_L", QVariantList() << "EyesWide_Left" << 1.0);
        blendshapes.insert("EyeOpen_R", QVariantList() << "EyesWide_Right" << 1.0);
        blendshapes.insert("EyeSquint_L", QVariantList() << "Squint_Left" << 1.0);
        blendshapes.insert("EyeSquint_R", QVariantList() << "Squint_Right" << 1.0);
        blendshapes.insert("JawFwd", QVariantList() << "JawForeward" << 1.0);
        blendshapes.insert("JawLeft", QVariantList() << "JawRotateY_Left" << 0.5);
        blendshapes.insert("JawOpen", QVariantList() << "MouthOpen" << 0.7);
        blendshapes.insert("JawRight", QVariantList() << "Jaw_Right" << 1.0);
        blendshapes.insert("LipsFunnel", QVariantList() << "JawForeward" << 0.39);
        blendshapes.insert("LipsFunnel", QVariantList() << "Jaw_Down" << 0.36);
        blendshapes.insert("LipsFunnel", QVariantList() << "MouthNarrow_Left" << 1.0);
        blendshapes.insert("LipsFunnel", QVariantList() << "MouthNarrow_Right" << 1.0);
        blendshapes.insert("LipsFunnel", QVariantList() << "MouthWhistle_NarrowAdjust_Left" << 0.5);
        blendshapes.insert("LipsFunnel", QVariantList() << "MouthWhistle_NarrowAdjust_Right" << 0.5);
        blendshapes.insert("LipsFunnel", QVariantList() << "TongueUp" << 1.0);
        blendshapes.insert("LipsLowerClose", QVariantList() << "LowerLipIn" << 1.0);
        blendshapes.insert("LipsLowerDown", QVariantList() << "LowerLipDown_Left" << 0.7);
        blendshapes.insert("LipsLowerDown", QVariantList() << "LowerLipDown_Right" << 0.7);
        blendshapes.insert("LipsLowerOpen", QVariantList() << "LowerLipOut" << 1.0);
        blendshapes.insert("LipsPucker", QVariantList() << "MouthNarrow_Left" << 1.0);
        blendshapes.insert("LipsPucker", QVariantList() << "MouthNarrow_Right" << 1.0);
        blendshapes.insert("LipsUpperClose", QVariantList() << "UpperLipIn" << 1.0);
        blendshapes.insert("LipsUpperOpen", QVariantList() << "UpperLipOut" << 1.0);
        blendshapes.insert("LipsUpperUp", QVariantList() << "UpperLipUp_Left" << 0.7);
        blendshapes.insert("LipsUpperUp", QVariantList() << "UpperLipUp_Right" << 0.7);
        blendshapes.insert("MouthDimple_L", QVariantList() << "Smile_Left" << 0.25);
        blendshapes.insert("MouthDimple_R", QVariantList() << "Smile_Right" << 0.25);
        blendshapes.insert("MouthFrown_L", QVariantList() << "Frown_Left" << 1.0);
        blendshapes.insert("MouthFrown_R", QVariantList() << "Frown_Right" << 1.0);
        blendshapes.insert("MouthLeft", QVariantList() << "Midmouth_Left" << 1.0);
        blendshapes.insert("MouthRight", QVariantList() << "Midmouth_Right" << 1.0);
        blendshapes.insert("MouthSmile_L", QVariantList() << "Smile_Left" << 1.0);
        blendshapes.insert("MouthSmile_R", QVariantList() << "Smile_Right" << 1.0);
        blendshapes.insert("Puff", QVariantList() << "CheekPuff_Left" << 1.0);
        blendshapes.insert("Puff", QVariantList() << "CheekPuff_Right" << 1.0);
        blendshapes.insert("Sneer", QVariantList() << "NoseScrunch_Left" << 0.75);
        blendshapes.insert("Sneer", QVariantList() << "NoseScrunch_Right" << 0.75);
        blendshapes.insert("Sneer", QVariantList() << "Squint_Left" << 0.5);
        blendshapes.insert("Sneer", QVariantList() << "Squint_Right" << 0.5);
        mapping.insert(BLENDSHAPE_FIELD, blendshapes);
    }
    return new FST(fstPath, mapping);
}

QString FST::absoluteModelPath() const {
    QFileInfo fileInfo{ _fstPath };
    QDir dir{ fileInfo.absoluteDir() };
    return dir.absoluteFilePath(_modelPath);
}

void FST::setName(const QString& name) {
    _name = name;
    emit nameChanged(name);
}

void FST::setModelPath(const QString& modelPath) {
    _modelPath = modelPath;
    emit modelPathChanged(modelPath);
}

QMultiHash<QString, QVariant> FST::getMapping() const {
    QMultiHash<QString, QVariant> mapping;
    mapping.unite(_other);
    mapping.insert(NAME_FIELD, _name);
    mapping.insert(FILENAME_FIELD, _modelPath);
    mapping.insert(MARKETPLACE_ID_FIELD, _marketplaceID);
    for (const auto& scriptPath : _scriptPaths) {
        mapping.insert(SCRIPT_FIELD, scriptPath);
    }
    return mapping;
}

bool FST::write() {
    QFile fst(_fstPath);
    if (!fst.open(QIODevice::WriteOnly)) {
        return false;
    }
    fst.write(FSTReader::writeMapping(getMapping()));
    return true;
}

void FST::setMarketplaceID(QUuid marketplaceID) {
    _marketplaceID = marketplaceID;
    emit marketplaceIDChanged();
}
