//
//  VoxelImporter.cpp
//  hifi
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <VoxelImporter.h>

#include <QFileInfo>
#include <QThreadPool>

class LocalVoxelSystem : public VoxelSystem {
public:
    LocalVoxelSystem() : VoxelSystem(1) {};

    virtual void removeOutOfView() {};
};

VoxelImporter::VoxelImporter(QWidget* parent)
    : QObject(parent),
      _voxelSystem(new LocalVoxelSystem()),
      _importDialog(parent, _voxelSystem),
      _currentTask(NULL),
      _nextTask(NULL) {

    connect(&_importDialog, SIGNAL(previewActivated(QString)), SLOT(preImport(QString)));
    connect(&_importDialog, SIGNAL(currentChanged(QString))  , SLOT(preImport(QString)));
}

void VoxelImporter::import(const QString &filename) {
    _importDialog.reset();
    _filename = filename;
    _currentTask = NULL;
    _nextTask = NULL;

    if (_filename == "") {
        _importDialog.exec();
    }


}

void VoxelImporter::preImport(const QString &filename) {
    if (_importDialog.getWantPreview() && QFileInfo(filename).isFile()) {
        _filename = filename;

        _nextTask = new ImportTask(_voxelSystem, _filename);
        connect(_nextTask, SIGNAL(destroyed()), SLOT(launchTask()));

        _importDialog.reset();

        if (_currentTask != NULL) {
            _voxelSystem->getVoxelTree()->cancelImport();
        } else {
            launchTask();
        }
    }
}

void VoxelImporter::launchTask() {
    if (_nextTask != NULL) {
        _voxelSystem->killLocalVoxels();
        _currentTask = _nextTask;
        _nextTask = NULL;
        QThreadPool::globalInstance()->start(_currentTask);
    } else {
        _currentTask = NULL;
    }
}

ImportTask::ImportTask(VoxelSystem* voxelSystem, const QString &filename)
    : _voxelSystem(voxelSystem),
      _filename(filename) {

}

void ImportTask::run() {
    if (_filename.endsWith(".png", Qt::CaseInsensitive)) {
        _voxelSystem->readFromSquareARGB32Pixels(_filename.toLocal8Bit().data());
    } else if (_filename.endsWith(".svo", Qt::CaseInsensitive)) {
        _voxelSystem->readFromSVOFile(_filename.toLocal8Bit().data());
    } else if (_filename.endsWith(".schematic", Qt::CaseInsensitive)) {
        qDebug("[DEBUG] %d.\n",
               _voxelSystem->readFromSchematicFile(_filename.toLocal8Bit().data()));
    } else {
        qDebug("[ERROR] Invalid file extension.\n");
    }
}
