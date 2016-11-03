//
//  ModelPackager.cpp
//
//
//  Created by Clement on 3/9/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTemporaryDir>

#include <FSTReader.h>
#include <OffscreenUi.h>

#include "ModelSelector.h"
#include "ModelPropertiesDialog.h"
#include "InterfaceLogging.h"

#include "ModelPackager.h"

static const int MAX_TEXTURE_SIZE = 1024;

void copyDirectoryContent(QDir& from, QDir& to) {
    for (auto entry : from.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot |
                                         QDir::NoSymLinks | QDir::Readable)) {
        if (entry.isDir()) {
            to.mkdir(entry.fileName());
            from.cd(entry.fileName());
            to.cd(entry.fileName());
            copyDirectoryContent(from, to);
            from.cdUp();
            to.cdUp();
        } else { // Files
            QFile file(entry.absoluteFilePath());
            QString newPath = to.absolutePath() + "/" + entry.fileName();
            if (to.exists(entry.fileName())) {
                QFile overridenFile(newPath);
                overridenFile.remove();
            }
            file.copy(newPath);
        }
    }
}

bool ModelPackager::package() {
    ModelPackager packager;
    if (!packager.selectModel()) {
        return false;
    }
    if (!packager.loadModel()) {
        return false;
    }
    if (!packager.editProperties()) {
        return false;
    }
    if (!packager.zipModel()) {
        return false;
    }
    return true;
}

bool ModelPackager::selectModel() {
    ModelSelector selector;
    if(selector.exec() == QDialog::Accepted) {
        _modelFile = selector.getFileInfo();
        _modelType = selector.getModelType();
        return true;
    }
    return false;
}

bool ModelPackager::loadModel() {
    // First we check the FST file (if any)
    if (_modelFile.completeSuffix().contains("fst")) {
        QFile fst(_modelFile.filePath());
        if (!fst.open(QFile::ReadOnly | QFile::Text)) {
            OffscreenUi::warning(NULL,
                                 QString("ModelPackager::loadModel()"),
                                 QString("Could not open FST file %1").arg(_modelFile.filePath()),
                                 QMessageBox::Ok);
            qWarning() << QString("ModelPackager::loadModel(): Could not open FST file %1").arg(_modelFile.filePath());
            return false;
        }
        qCDebug(interfaceapp) << "Reading FST file : " << _modelFile.filePath();
        _mapping = FSTReader::readMapping(fst.readAll());
        fst.close();
        
        _fbxInfo = QFileInfo(_modelFile.path() + "/" + _mapping.value(FILENAME_FIELD).toString());
    } else {
        _fbxInfo = QFileInfo(_modelFile.filePath());
    }
    
    // open the fbx file
    QFile fbx(_fbxInfo.filePath());
    if (!_fbxInfo.exists() || !_fbxInfo.isFile() || !fbx.open(QIODevice::ReadOnly)) {
        OffscreenUi::warning(NULL,
                             QString("ModelPackager::loadModel()"),
                             QString("Could not open FBX file %1").arg(_fbxInfo.filePath()),
                             QMessageBox::Ok);
        qWarning() << QString("ModelPackager::loadModel(): Could not open FBX file %1").arg(_fbxInfo.filePath());
        return false;
    }
    try {
        qCDebug(interfaceapp) << "Reading FBX file : " << _fbxInfo.filePath();
        QByteArray fbxContents = fbx.readAll();

        _geometry.reset(readFBX(fbxContents, QVariantHash(), _fbxInfo.filePath()));

        // make sure we have some basic mappings
        populateBasicMapping(_mapping, _fbxInfo.filePath(), *_geometry);
    } catch (const QString& error) {
        qCDebug(interfaceapp) << "Error reading " << _fbxInfo.filePath() << ": " << error;
        return false;
    }
    return true;
}

bool ModelPackager::editProperties() {
    // open the dialog to configure the rest
    ModelPropertiesDialog properties(_modelType, _mapping, _modelFile.path(), *_geometry);
    if (properties.exec() == QDialog::Rejected) {
        return false;
    }
    _mapping = properties.getMapping();

    if (_modelType == FSTReader::BODY_ONLY_MODEL || _modelType == FSTReader::HEAD_AND_BODY_MODEL) {
        // Make sure that a mapping for the root joint has been specified
        QVariantHash joints = _mapping.value(JOINT_FIELD).toHash();
        if (!joints.contains("jointRoot")) {
            qWarning() << QString("%1 root joint not configured for skeleton.").arg(_modelFile.fileName());
        
            QString message = "Your did not configure a root joint for your skeleton model.\n\nPackaging will be canceled.";
            QMessageBox msgBox;
            msgBox.setWindowTitle("Model Packager");
            msgBox.setText(message);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
        
            return false;
        }
    }
    
    return true;
}

bool ModelPackager::zipModel() {
    QTemporaryDir dir;
    dir.setAutoRemove(true);
    QDir tempDir(dir.path());
    
    QByteArray nameField = _mapping.value(NAME_FIELD).toByteArray();
    tempDir.mkpath(nameField + "/textures");
    QDir fbxDir(tempDir.path() + "/" + nameField);
    QDir texDir(fbxDir.path() + "/textures");
    
    // Copy textures
    listTextures();
    if (!_textures.empty()) {
        QByteArray texdirField = _mapping.value(TEXDIR_FIELD).toByteArray();
        _texDir = _modelFile.path() + "/" + texdirField;
        copyTextures(_texDir, texDir);
    }
    
    // Copy LODs
    QVariantHash lodField = _mapping.value(LOD_FIELD).toHash();
    if (!lodField.empty()) {
        for (auto it = lodField.constBegin(); it != lodField.constEnd(); ++it) {
            QString oldPath = _modelFile.path() + "/" + it.key();
            QFile lod(oldPath);
            QString newPath = fbxDir.path() + "/" + QFileInfo(lod).fileName();
            if (lod.exists()) {
                lod.copy(newPath);
            }
        }
    }
    
    // Copy FBX
    QFile fbx(_fbxInfo.filePath());
    QByteArray filenameField = _mapping.value(FILENAME_FIELD).toByteArray();
    QString newPath = fbxDir.path() + "/" + QFileInfo(filenameField).fileName();
    fbx.copy(newPath);
    
    // Correct FST
    _mapping[FILENAME_FIELD] = tempDir.relativeFilePath(newPath);
    _mapping[TEXDIR_FIELD] = tempDir.relativeFilePath(texDir.path());
    
    // Copy FST
    QFile fst(tempDir.path() + "/" + nameField + ".fst");
    if (fst.open(QIODevice::WriteOnly)) {
        fst.write(FSTReader::writeMapping(_mapping));
        fst.close();
    } else {
        qCDebug(interfaceapp) << "Couldn't write FST file" << fst.fileName();
        return false;
    }
    
    
    QString saveDirPath = QFileDialog::getExistingDirectory(nullptr, "Save Model",
                                                        "", QFileDialog::ShowDirsOnly);
    if (saveDirPath.isEmpty()) {
        qCDebug(interfaceapp) << "Invalid directory" << saveDirPath;
        return false;
    }
    
    QDir saveDir(saveDirPath);
    copyDirectoryContent(tempDir, saveDir);
    return true;
}

void ModelPackager::populateBasicMapping(QVariantHash& mapping, QString filename, const FBXGeometry& geometry) {

    bool isBodyType = _modelType == FSTReader::BODY_ONLY_MODEL || _modelType == FSTReader::HEAD_AND_BODY_MODEL;

    // mixamo files - in the event that a mixamo file was edited by some other tool, it's likely the applicationName will
    // be rewritten, so we detect the existence of several different blendshapes which indicate we're likely a mixamo file
    bool likelyMixamoFile = geometry.applicationName == "mixamo.com" ||
                            (geometry.blendshapeChannelNames.contains("BrowsDown_Right") &&
                             geometry.blendshapeChannelNames.contains("MouthOpen") &&
                             geometry.blendshapeChannelNames.contains("Blink_Left") &&
                             geometry.blendshapeChannelNames.contains("Blink_Right") &&
                             geometry.blendshapeChannelNames.contains("Squint_Right"));
    
    if (!mapping.contains(NAME_FIELD)) {
        mapping.insert(NAME_FIELD, QFileInfo(filename).baseName());
    }
    
    if (!mapping.contains(FILENAME_FIELD)) {
        QDir root(_modelFile.path());
        mapping.insert(FILENAME_FIELD, root.relativeFilePath(filename));
    }
    if (!mapping.contains(TEXDIR_FIELD)) {
        mapping.insert(TEXDIR_FIELD, ".");
    }
    
    // mixamo/autodesk defaults
    if (!mapping.contains(SCALE_FIELD)) {
        mapping.insert(SCALE_FIELD, 1.0);
    }
    QVariantHash joints = mapping.value(JOINT_FIELD).toHash();
    if (!joints.contains("jointEyeLeft")) {
        joints.insert("jointEyeLeft", geometry.jointIndices.contains("jointEyeLeft") ? "jointEyeLeft" :
                      (geometry.jointIndices.contains("EyeLeft") ? "EyeLeft" : "LeftEye"));
    }
    if (!joints.contains("jointEyeRight")) {
        joints.insert("jointEyeRight", geometry.jointIndices.contains("jointEyeRight") ? "jointEyeRight" :
                      geometry.jointIndices.contains("EyeRight") ? "EyeRight" : "RightEye");
    }
    if (!joints.contains("jointNeck")) {
        joints.insert("jointNeck", geometry.jointIndices.contains("jointNeck") ? "jointNeck" : "Neck");
    }
    
    if (isBodyType) {
        if (!joints.contains("jointRoot")) {
            joints.insert("jointRoot", "Hips");
        }
        if (!joints.contains("jointLean")) {
            joints.insert("jointLean", "Spine");
        }
        if (!joints.contains("jointLeftHand")) {
            joints.insert("jointLeftHand", "LeftHand");
        }
        if (!joints.contains("jointRightHand")) {
            joints.insert("jointRightHand", "RightHand");
        }
    }
    
    if (!joints.contains("jointHead")) {
        const char* topName = likelyMixamoFile ? "HeadTop_End" : "HeadEnd";
        joints.insert("jointHead", geometry.jointIndices.contains(topName) ? topName : "Head");
    }

    mapping.insert(JOINT_FIELD, joints);

    if (isBodyType) {
        if (!mapping.contains(FREE_JOINT_FIELD)) {
            mapping.insertMulti(FREE_JOINT_FIELD, "LeftArm");
            mapping.insertMulti(FREE_JOINT_FIELD, "LeftForeArm");
            mapping.insertMulti(FREE_JOINT_FIELD, "RightArm");
            mapping.insertMulti(FREE_JOINT_FIELD, "RightForeArm");
        }
    }
    
    // If there are no blendshape mappings, and we detect that this is likely a mixamo file,
    // then we can add the default mixamo to "faceshift" mappings
    if (!mapping.contains(BLENDSHAPE_FIELD) && likelyMixamoFile) {
        QVariantHash blendshapes;
        blendshapes.insertMulti("BrowsD_L", QVariantList() << "BrowsDown_Left" << 1.0);
        blendshapes.insertMulti("BrowsD_R", QVariantList() << "BrowsDown_Right" << 1.0);
        blendshapes.insertMulti("BrowsU_C", QVariantList() << "BrowsUp_Left" << 1.0);
        blendshapes.insertMulti("BrowsU_C", QVariantList() << "BrowsUp_Right" << 1.0);
        blendshapes.insertMulti("BrowsU_L", QVariantList() << "BrowsUp_Left" << 1.0);
        blendshapes.insertMulti("BrowsU_R", QVariantList() << "BrowsUp_Right" << 1.0);
        blendshapes.insertMulti("ChinLowerRaise", QVariantList() << "Jaw_Up" << 1.0);
        blendshapes.insertMulti("ChinUpperRaise", QVariantList() << "UpperLipUp_Left" << 0.5);
        blendshapes.insertMulti("ChinUpperRaise", QVariantList() << "UpperLipUp_Right" << 0.5);
        blendshapes.insertMulti("EyeBlink_L", QVariantList() << "Blink_Left" << 1.0);
        blendshapes.insertMulti("EyeBlink_R", QVariantList() << "Blink_Right" << 1.0);
        blendshapes.insertMulti("EyeOpen_L", QVariantList() << "EyesWide_Left" << 1.0);
        blendshapes.insertMulti("EyeOpen_R", QVariantList() << "EyesWide_Right" << 1.0);
        blendshapes.insertMulti("EyeSquint_L", QVariantList() << "Squint_Left" << 1.0);
        blendshapes.insertMulti("EyeSquint_R", QVariantList() << "Squint_Right" << 1.0);
        blendshapes.insertMulti("JawFwd", QVariantList() << "JawForeward" << 1.0);
        blendshapes.insertMulti("JawLeft", QVariantList() << "JawRotateY_Left" << 0.5);
        blendshapes.insertMulti("JawOpen", QVariantList() << "MouthOpen" << 0.7);
        blendshapes.insertMulti("JawRight", QVariantList() << "Jaw_Right" << 1.0);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "JawForeward" << 0.39);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "Jaw_Down" << 0.36);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthNarrow_Left" << 1.0);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthNarrow_Right" << 1.0);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthWhistle_NarrowAdjust_Left" << 0.5);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthWhistle_NarrowAdjust_Right" << 0.5);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "TongueUp" << 1.0);
        blendshapes.insertMulti("LipsLowerClose", QVariantList() << "LowerLipIn" << 1.0);
        blendshapes.insertMulti("LipsLowerDown", QVariantList() << "LowerLipDown_Left" << 0.7);
        blendshapes.insertMulti("LipsLowerDown", QVariantList() << "LowerLipDown_Right" << 0.7);
        blendshapes.insertMulti("LipsLowerOpen", QVariantList() << "LowerLipOut" << 1.0);
        blendshapes.insertMulti("LipsPucker", QVariantList() << "MouthNarrow_Left" << 1.0);
        blendshapes.insertMulti("LipsPucker", QVariantList() << "MouthNarrow_Right" << 1.0);
        blendshapes.insertMulti("LipsUpperClose", QVariantList() << "UpperLipIn" << 1.0);
        blendshapes.insertMulti("LipsUpperOpen", QVariantList() << "UpperLipOut" << 1.0);
        blendshapes.insertMulti("LipsUpperUp", QVariantList() << "UpperLipUp_Left" << 0.7);
        blendshapes.insertMulti("LipsUpperUp", QVariantList() << "UpperLipUp_Right" << 0.7);
        blendshapes.insertMulti("MouthDimple_L", QVariantList() << "Smile_Left" << 0.25);
        blendshapes.insertMulti("MouthDimple_R", QVariantList() << "Smile_Right" << 0.25);
        blendshapes.insertMulti("MouthFrown_L", QVariantList() << "Frown_Left" << 1.0);
        blendshapes.insertMulti("MouthFrown_R", QVariantList() << "Frown_Right" << 1.0);
        blendshapes.insertMulti("MouthLeft", QVariantList() << "Midmouth_Left" << 1.0);
        blendshapes.insertMulti("MouthRight", QVariantList() << "Midmouth_Right" << 1.0);
        blendshapes.insertMulti("MouthSmile_L", QVariantList() << "Smile_Left" << 1.0);
        blendshapes.insertMulti("MouthSmile_R", QVariantList() << "Smile_Right" << 1.0);
        blendshapes.insertMulti("Puff", QVariantList() << "CheekPuff_Left" << 1.0);
        blendshapes.insertMulti("Puff", QVariantList() << "CheekPuff_Right" << 1.0);
        blendshapes.insertMulti("Sneer", QVariantList() << "NoseScrunch_Left" << 0.75);
        blendshapes.insertMulti("Sneer", QVariantList() << "NoseScrunch_Right" << 0.75);
        blendshapes.insertMulti("Sneer", QVariantList() << "Squint_Left" << 0.5);
        blendshapes.insertMulti("Sneer", QVariantList() << "Squint_Right" << 0.5);
        mapping.insert(BLENDSHAPE_FIELD, blendshapes);
    }
}

void ModelPackager::listTextures() {
    _textures.clear();
    foreach (const FBXMaterial mat, _geometry->materials) {
        if (!mat.albedoTexture.filename.isEmpty() && mat.albedoTexture.content.isEmpty() &&
            !_textures.contains(mat.albedoTexture.filename)) {
            _textures << mat.albedoTexture.filename;
        }
        if (!mat.normalTexture.filename.isEmpty() && mat.normalTexture.content.isEmpty() &&
            !_textures.contains(mat.normalTexture.filename)) {
                
            _textures << mat.normalTexture.filename;
        }
        if (!mat.specularTexture.filename.isEmpty() && mat.specularTexture.content.isEmpty() &&
            !_textures.contains(mat.specularTexture.filename)) {
            _textures << mat.specularTexture.filename;
        }
        if (!mat.emissiveTexture.filename.isEmpty() && mat.emissiveTexture.content.isEmpty() &&
            !_textures.contains(mat.emissiveTexture.filename)) {
            _textures << mat.emissiveTexture.filename;
        }
    }
}

bool ModelPackager::copyTextures(const QString& oldDir, const QDir& newDir) {
    QString errors;
    for (auto texture : _textures) {
        QString oldPath = oldDir + "/" + texture;
        QString newPath = newDir.path() + "/" + texture;

        // Make sure path exists
        if (texture.contains("/")) {
            QString dirPath = newDir.relativeFilePath(QFileInfo(newPath).path());
            newDir.mkpath(dirPath);
        }
        
        QFile texFile(oldPath);
        if (texFile.exists() && texFile.open(QIODevice::ReadOnly)) {
            // Check if texture needs to be recoded
            QFileInfo fileInfo(oldPath);
            QString extension = fileInfo.suffix().toLower();
            bool isJpeg = (extension == "jpg");
            bool mustRecode = !(isJpeg || extension == "png");
            QImage image = QImage::fromData(texFile.readAll());

            // Recode texture if too big
            if (image.width() > MAX_TEXTURE_SIZE || image.height() > MAX_TEXTURE_SIZE) {
                image = image.scaled(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, Qt::KeepAspectRatio);
                mustRecode = true;
            }
            
            // Copy texture
            if (mustRecode) {
                QFile newTexFile(newPath);
                newTexFile.open(QIODevice::WriteOnly);
                image.save(&newTexFile, isJpeg ? "JPG" : "PNG");
            } else {
                texFile.copy(newPath);
            }
        } else {
            errors += QString("\n%1").arg(oldPath);
        }
    }
    
    if (!errors.isEmpty()) {
        OffscreenUi::warning(nullptr, "ModelPackager::copyTextures()",
                             "Missing textures:" + errors);
        qCDebug(interfaceapp) << "ModelPackager::copyTextures():" << errors;
        return false;
    }
    
    return true;
}


