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

const QString SETTINGS_GROUP_NAME = "VoxelImport";
const QString IMPORT_DIALOG_SETTINGS_KEY = "ImportDialogSettings";

class ImportTask : public QObject, public QRunnable {
public:
    ImportTask(const QString &filename);
    void run();

private:
    QString _filename;
};

VoxelImporter::VoxelImporter(QWidget* parent) :
    QObject(parent),
    _voxelTree(true),
    _importDialog(parent),
    _task(NULL),
    _didImport(false)
{
    connect(&_voxelTree, SIGNAL(importProgress(int)), &_importDialog, SLOT(setProgressBarValue(int)));
    connect(&_importDialog, SIGNAL(canceled()), this, SLOT(cancel()));
    connect(&_importDialog, SIGNAL(accepted()), this, SLOT(import()));
}

void VoxelImporter::saveSettings(QSettings* settings) {
    settings->beginGroup(SETTINGS_GROUP_NAME);
    settings->setValue(IMPORT_DIALOG_SETTINGS_KEY, _importDialog.saveState());
    settings->endGroup();
}

void VoxelImporter::loadSettings(QSettings* settings) {
    settings->beginGroup(SETTINGS_GROUP_NAME);
    _importDialog.restoreState(settings->value(IMPORT_DIALOG_SETTINGS_KEY).toByteArray());
    settings->endGroup();
}

VoxelImporter::~VoxelImporter() {
    cleanupTask();
}

void VoxelImporter::reset() {
    _voxelTree.eraseAllOctreeElements();
    _importDialog.reset();

    cleanupTask();
}

int VoxelImporter::exec() {
    reset();
    _importDialog.exec();
    
    if (!_didImport) {
        // if the import is rejected, we make sure to cleanup before leaving
        cleanupTask();
        return 1;
    } else {
        _didImport = false;
        return 0;
    }
}

void VoxelImporter::import() {
    switch (_importDialog.getMode()) {
        case loadingMode:
            _importDialog.setMode(placeMode);
            return;
        case placeMode:
            // Means the user chose to import
            _didImport = true;
            _importDialog.close();
            return;
        case importMode:
        default:
            QString filename = _importDialog.getCurrentFile();
            // if it's not a file, we ignore the call
            if (!QFileInfo(filename).isFile()) {
                return;
            }
            
            // Let's prepare the dialog window for import
            _importDialog.setMode(loadingMode);
            
            // If not already done, we switch to the local tree
            if (Application::getInstance()->getSharedVoxelSystem()->getTree() != &_voxelTree) {
                Application::getInstance()->getSharedVoxelSystem()->changeTree(&_voxelTree);
            }
            
            // Creation and launch of the import task on the thread pool
            _task = new ImportTask(filename);
            connect(_task, SIGNAL(destroyed()), SLOT(import()));
            QThreadPool::globalInstance()->start(_task);
            break;
    }
}

void VoxelImporter::cancel() {
    switch (_importDialog.getMode()) {
        case loadingMode:
            disconnect(_task, 0, 0, 0);
            cleanupTask();
        case placeMode:
            _importDialog.setMode(importMode);
            break;
        case importMode:
        default:
            _importDialog.close();
            break;
    }
}

void VoxelImporter::cleanupTask() {
    // If a task is running, we cancel it and put the pointer to null
    if (_task) {
        _task = NULL;
        _voxelTree.cancelImport();
    }
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

    // Then we call the righ method for the job
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
    
    // Here we reaverage the tree so that he is ready for preview
    voxelSystem->getTree()->reaverageOctreeElements();
}
