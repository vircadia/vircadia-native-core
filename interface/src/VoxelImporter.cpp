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

static const int MAX_VOXELS_PER_IMPORT = 2000000;

class ImportTask : public QObject, public QRunnable {
public:
    ImportTask(VoxelSystem* voxelSystem, const QString &filename);
    void run();

private:
    VoxelSystem*  _voxelSystem;
    QString       _filename;
};

VoxelImporter::VoxelImporter(QWidget* parent)
    : QObject(parent),
      _voxelSystem(new VoxelSystem(1, MAX_VOXELS_PER_IMPORT)),
      _initialized(false),
      _importWaiting(false),
      _importDialog(parent, _voxelSystem),
      _currentTask(NULL),
      _nextTask(NULL) {

    connect(&_importDialog, SIGNAL(previewToggled(bool)), SLOT(preImport()));
    connect(&_importDialog, SIGNAL(currentChanged(QString)), SLOT(preImport()));
    connect(&_importDialog, SIGNAL(accepted()), SLOT(import()));
}

VoxelImporter::~VoxelImporter() {
    if (_nextTask) {
        delete _nextTask;
        _nextTask = NULL;
    }

    if (_currentTask) {
        disconnect(_currentTask, 0, 0, 0);
        connect(_currentTask, SIGNAL(destroyed()), _voxelSystem, SLOT(deleteLater()));
        _voxelSystem->cancelImport();
        _currentTask = NULL;
    } else {
        delete _voxelSystem;
    }
}

void VoxelImporter::reset() {
    _voxelSystem->killLocalVoxels();
    _importDialog.reset();
    _filename = "";
    _importWaiting = false;

    if (_nextTask) {
        delete _nextTask;
        _nextTask = NULL;
    }

    if (_currentTask) {
        _voxelSystem->cancelImport();
    }
}

int VoxelImporter::exec() {
    reset();

    if (_initialized) {
        _voxelSystem->init();
        _importViewFrustum.setKeyholeRadius(100000.0f);
        _importViewFrustum.calculate();
        _voxelSystem->setViewFrustum(&_importViewFrustum);
        _initialized = true;
    }

    int ret = _importDialog.exec();

    if (!ret) {
        reset();
    } else {
        _importDialog.reset();
        _importWaiting = true;
    }

    return ret;
}

int VoxelImporter::preImport() {
    QString filename = _importDialog.getCurrentFile();

    if (!QFileInfo(filename).isFile()) {
        return 0;
    }

    if (_importDialog.getWantPreview()) {
        _filename = filename;

        if (_nextTask) {
            delete _nextTask;
        }

        _nextTask = new ImportTask(_voxelSystem, _filename);
        connect(_nextTask, SIGNAL(destroyed()), SLOT(launchTask()));

        if (_currentTask != NULL) {
            _voxelSystem->cancelImport();
        } else {
            launchTask();
        }
    }

    return 1;
}

int VoxelImporter::import() {
    QString filename = _importDialog.getCurrentFile();

    if (!QFileInfo(filename).isFile()) {
        _importDialog.reject();
        return 0;
    }

    if (_filename == filename) {
        if (_currentTask) {
            connect(_currentTask, SIGNAL(destroyed()), &_importDialog, SLOT(accept()));
        } else {
            _importDialog.accept();
        }
        return 1;
    }

    _filename = filename;

    if (_nextTask) {
        delete _nextTask;
    }

    _nextTask = new ImportTask(_voxelSystem, _filename);
    connect(_nextTask, SIGNAL(destroyed()), SLOT(launchTask()));
    connect(_nextTask, SIGNAL(destroyed()), &_importDialog, SLOT(accept()));

    if (_currentTask != NULL) {
        _voxelSystem->cancelImport();
    } else {
        launchTask();
    }

    return 1;
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
        _voxelSystem->readFromSchematicFile(_filename.toLocal8Bit().data());
    } else {
        qDebug("[ERROR] Invalid file extension.\n");
    }
}
