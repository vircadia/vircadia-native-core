//
//  VoxelImporter.cpp
//  hifi
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <VoxelImporter.h>
#include <Application.h>

#include <QFileInfo>
#include <QThreadPool>

class ImportTask : public QObject, public QRunnable {
public:
    ImportTask(const QString &filename);
    void run();

private:
    QString _filename;
};

VoxelImporter::VoxelImporter(QWidget* parent)
    : QObject(parent),
      _voxelTree(true),
      _importDialog(parent),
      _currentTask(NULL),
      _nextTask(NULL) {

    connect(&_importDialog, SIGNAL(previewToggled(bool)), SLOT(preImport()));
    connect(&_importDialog, SIGNAL(currentChanged(QString)), SLOT(preImport()));
    connect(&_importDialog, SIGNAL(accepted()), SLOT(import()));
}

void VoxelImporter::init() {
    _importDialog.init();
}

VoxelImporter::~VoxelImporter() {
    if (_nextTask) {
        delete _nextTask;
        _nextTask = NULL;
    }

    if (_currentTask) {
        disconnect(_currentTask, 0, 0, 0);
        _voxelTree.cancelImport();
        _currentTask = NULL;
    }
}

void VoxelImporter::reset() {
    _voxelTree.eraseAllOctreeElements();
    _importDialog.reset();
    _filename = "";

    if (_nextTask) {
        delete _nextTask;
        _nextTask = NULL;
    }

    if (_currentTask) {
        _voxelTree.cancelImport();
    }
}

int VoxelImporter::exec() {
    reset();

    int ret = _importDialog.exec();

    if (!ret) {
        reset();
    } else {
        _importDialog.reset();

        if (_importDialog.getImportIntoClipboard()) {
            VoxelSystem* voxelSystem = Application::getInstance()->getSharedVoxelSystem();

            voxelSystem->copySubTreeIntoNewTree(voxelSystem->getTree()->getRoot(),
                                                Application::getInstance()->getClipboard(),
                                                true);
            voxelSystem->changeTree(Application::getInstance()->getClipboard());
        }
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

        _nextTask = new ImportTask(_filename);
        connect(_nextTask, SIGNAL(destroyed()), SLOT(launchTask()));

        if (_currentTask != NULL) {
            _voxelTree.cancelImport();
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

    _nextTask = new ImportTask(_filename);
    connect(_nextTask, SIGNAL(destroyed()), SLOT(launchTask()));
    connect(_nextTask, SIGNAL(destroyed()), &_importDialog, SLOT(accept()));

    if (_currentTask != NULL) {
        _voxelTree.cancelImport();
    } else {
        launchTask();
    }

    return 1;
}

void VoxelImporter::launchTask() {
    if (_nextTask != NULL) {
        _currentTask = _nextTask;
        _nextTask = NULL;

        if (Application::getInstance()->getSharedVoxelSystem()->getTree() != &_voxelTree) {
            Application::getInstance()->getSharedVoxelSystem()->changeTree(&_voxelTree);
        }

        QThreadPool::globalInstance()->start(_currentTask);
    } else {
        _currentTask = NULL;
    }
}

ImportTask::ImportTask(const QString &filename)
    : _filename(filename) {
}

void ImportTask::run() {
    VoxelSystem* voxelSystem = Application::getInstance()->getSharedVoxelSystem();
    voxelSystem->killLocalVoxels();

    if (_filename.endsWith(".png", Qt::CaseInsensitive)) {
        voxelSystem->readFromSquareARGB32Pixels(_filename.toLocal8Bit().data());
    } else if (_filename.endsWith(".svo", Qt::CaseInsensitive)) {
        voxelSystem->readFromSVOFile(_filename.toLocal8Bit().data());
    } else if (_filename.endsWith(".schematic", Qt::CaseInsensitive)) {
        voxelSystem->readFromSchematicFile(_filename.toLocal8Bit().data());
    } else {
        qDebug("[ERROR] Invalid file extension.\n");
    }

    voxelSystem->getTree()->reaverageOctreeElements();
}
