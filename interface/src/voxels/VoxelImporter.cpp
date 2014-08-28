//
//  VoxelImporter.cpp
//  interface/src/voxels
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QFileInfo>
#include <QThreadPool>

#include <Application.h>
#include <LocalVoxelsList.h>

#include "voxels/VoxelImporter.h"

const QStringList SUPPORTED_EXTENSIONS = QStringList() << "png" << "svo" << "schematic";

class ImportTask : public QObject, public QRunnable {
public:
    ImportTask(const QString &filename);
    void run();

private:
    QString _filename;
};

VoxelImporter::VoxelImporter() :
    _voxelTree(true),
    _task(NULL)
{
    LocalVoxelsList::getInstance()->addPersistantTree(IMPORT_TREE_NAME, &_voxelTree);
    
    connect(&_voxelTree, SIGNAL(importProgress(int)), this, SIGNAL(importProgress(int)));
}

VoxelImporter::~VoxelImporter() {
    cleanupTask();
}

void VoxelImporter::cancel() {
    if (_task) {
        disconnect(_task, 0, 0, 0);
    }
    reset();
}

void VoxelImporter::reset() {
    _voxelTree.eraseAllOctreeElements();
    cleanupTask();
}

void VoxelImporter::import(const QString& filename) {
    // If present, abort existing import
    if (_task) {
        cleanupTask();
    }

    // If not already done, we switch to the local tree
    if (Application::getInstance()->getSharedVoxelSystem()->getTree() != &_voxelTree) {
        Application::getInstance()->getSharedVoxelSystem()->changeTree(&_voxelTree);
    }

    // Creation and launch of the import task on the thread pool
    _task = new ImportTask(filename);
    connect(_task, SIGNAL(destroyed()), SLOT(finishImport()));
    QThreadPool::globalInstance()->start(_task);
}

void VoxelImporter::cleanupTask() {
    // If a task is running, we cancel it and put the pointer to null
    if (_task) {
        _task = NULL;
        _voxelTree.cancelImport();
    }
}

void VoxelImporter::finishImport() {
    cleanupTask();
    emit importDone();
}

bool VoxelImporter::validImportFile(const QString& filename) {
    QFileInfo fileInfo = QFileInfo(filename);
    return fileInfo.isFile() && SUPPORTED_EXTENSIONS.indexOf(fileInfo.suffix().toLower()) != -1;
}

ImportTask::ImportTask(const QString &filename)
    : _filename(filename)
{
    setAutoDelete(true);
}

void ImportTask::run() {
    VoxelSystem* voxelSystem = Application::getInstance()->getSharedVoxelSystem();
    // We start by cleaning up the shared voxel system just in case
    voxelSystem->killLocalVoxels();

    // Then we call the right method for the job
    if (_filename.endsWith(".png", Qt::CaseInsensitive)) {
        voxelSystem->getTree()->readFromSquareARGB32Pixels(_filename.toLocal8Bit().data());
    } else if (_filename.endsWith(".svo", Qt::CaseInsensitive)) {
        voxelSystem->getTree()->readFromSVOFile(_filename.toLocal8Bit().data());
    } else if (_filename.endsWith(".schematic", Qt::CaseInsensitive)) {
        voxelSystem->getTree()->readFromSchematicFile(_filename.toLocal8Bit().data());
    } else {
        // We should never get here.
        qDebug() << "[ERROR] Invalid file extension." << endl;
    }
    
    // Here we reaverage the tree so that it is ready for preview
    voxelSystem->getTree()->reaverageOctreeElements();
}
