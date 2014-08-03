//
//  ClipboardScriptingInterface.cpp
//  interface/src/scripting
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "ClipboardScriptingInterface.h"

ClipboardScriptingInterface::ClipboardScriptingInterface() {
    connect(this, SIGNAL(readyToImport()), Application::getInstance(), SLOT(importVoxels()));
}

void ClipboardScriptingInterface::cutVoxel(const VoxelDetail& sourceVoxel) {
    cutVoxel(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
}

void ClipboardScriptingInterface::cutVoxel(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };
    Application::getInstance()->cutVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::copyVoxel(const VoxelDetail& sourceVoxel) {
    copyVoxel(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
}

void ClipboardScriptingInterface::copyVoxel(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };
    Application::getInstance()->copyVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::pasteVoxel(const VoxelDetail& destinationVoxel) {
    pasteVoxel(destinationVoxel.x, destinationVoxel.y, destinationVoxel.z, destinationVoxel.s);
}

void ClipboardScriptingInterface::pasteVoxel(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE, 
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };

    Application::getInstance()->pasteVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::deleteVoxel(const VoxelDetail& sourceVoxel) {
    deleteVoxel(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
}

void ClipboardScriptingInterface::deleteVoxel(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };
    Application::getInstance()->deleteVoxels(sourceVoxel);
}

void ClipboardScriptingInterface::exportVoxel(const VoxelDetail& sourceVoxel) {
    exportVoxel(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
}

void ClipboardScriptingInterface::exportVoxel(float x, float y, float z, float s) {
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };

    Application::getInstance()->exportVoxels(sourceVoxel);
}

bool ClipboardScriptingInterface::importVoxels() {
    qDebug() << "Importing ... ";
    QEventLoop loop;
    connect(Application::getInstance(), SIGNAL(importDone()), &loop, SLOT(quit()));
    emit readyToImport();
    loop.exec();
    
    return Application::getInstance()->getImportSucceded();
}

bool ClipboardScriptingInterface::importVoxels(const QString& filename) {
    qDebug() << "Importing ... ";

    VoxelImporter* importer = Application::getInstance()->getVoxelImporter();

    if (!importer->validImportFile(filename)) {
        return false;
    }

    QEventLoop loop;
    connect(importer, SIGNAL(importDone()), &loop, SLOT(quit()));
    importer->import(filename);
    loop.exec();

    return true;
}

bool ClipboardScriptingInterface::importVoxels(const QString& filename, float x, float y, float z, float s) {
    bool success = importVoxels(filename);

    if (success) {
        pasteVoxel(x, y, z, s);
    }

    return success;
}

bool ClipboardScriptingInterface::importVoxels(const QString& filename, const VoxelDetail& destinationVoxel) {
    return importVoxels(filename, destinationVoxel.x, destinationVoxel.y, destinationVoxel.z, destinationVoxel.s);
}

void ClipboardScriptingInterface::nudgeVoxel(const VoxelDetail& sourceVoxel, const glm::vec3& nudgeVec) {
    nudgeVoxel(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s, nudgeVec);
}

void ClipboardScriptingInterface::nudgeVoxel(float x, float y, float z, float s, const glm::vec3& nudgeVec) {
    glm::vec3 nudgeVecInTreeSpace = nudgeVec / (float)TREE_SCALE;
    VoxelDetail sourceVoxel = { x / (float)TREE_SCALE,
                                 y / (float)TREE_SCALE, 
                                 z / (float)TREE_SCALE, 
                                 s / (float)TREE_SCALE };

    Application::getInstance()->nudgeVoxelsByVector(sourceVoxel, nudgeVecInTreeSpace);
}
