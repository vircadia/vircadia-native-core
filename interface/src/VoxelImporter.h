//
//  VoxelImporter.h
//  hifi
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelImporter__
#define __hifi__VoxelImporter__

#include <QThread>
#include <QRunnable>

#include <VoxelSystem.h>

#include "ui/ImportDialog.h"

class ImportTask;

class VoxelImporter : public QObject {
    Q_OBJECT
public:
    VoxelImporter(QWidget* parent = NULL);
    ~VoxelImporter();

    void reset();
    void loadSettings(QSettings* settings);
    void saveSettings(QSettings* settings);

    VoxelTree* getVoxelTree() { return &_voxelTree; }

public slots:
    int exec();
    void import();
    void cancel();

private:
    VoxelTree _voxelTree;
    ImportDialog _importDialog;

    ImportTask* _task;
    bool _didImport;
    
    void cleanupTask();
};

#endif /* defined(__hifi__VoxelImporter__) */
