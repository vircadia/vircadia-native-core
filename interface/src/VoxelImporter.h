//
//  VoxelImporter.h
//  hifi
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelImporter__
#define __hifi__VoxelImporter__

#include <VoxelSystem.h>
#include <ImportDialog.h>

#include <QThread>
#include <QRunnable>

class ImportTask;
class LocalVoxelSystem;

class VoxelImporter : public QObject {
    Q_OBJECT
public:
    VoxelImporter(QWidget* parent = NULL);
    ~VoxelImporter();
    void reset();

    VoxelSystem* getVoxelSystem() const { return _voxelSystem; }
    bool getimportIntoClipboard() const { return _importDialog.getImportIntoClipboard(); }

public slots:
    int exec();
    int preImport();
    int import();

private slots:
    void launchTask();

private:
    VoxelSystem* _voxelSystem;

    ImportDialog _importDialog;

    QString      _filename;

    ImportTask* _currentTask;
    ImportTask* _nextTask;
};

#endif /* defined(__hifi__VoxelImporter__) */
