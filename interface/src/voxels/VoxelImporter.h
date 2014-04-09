//
//  VoxelImporter.h
//  interface/src/voxels
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelImporter_h
#define hifi_VoxelImporter_h

#include <QThread>
#include <QRunnable>

#include "ui/ImportDialog.h"
#include "voxels/VoxelSystem.h"

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

#endif // hifi_VoxelImporter_h
