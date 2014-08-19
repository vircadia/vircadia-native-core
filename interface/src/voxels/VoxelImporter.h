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
#include <QStringList>

#include "voxels/VoxelSystem.h"

class ImportTask;

class VoxelImporter : public QObject {
    Q_OBJECT
public:
    VoxelImporter();
    ~VoxelImporter();

    void reset();
    void cancel();
    VoxelTree* getVoxelTree() { return &_voxelTree; }
    bool validImportFile(const QString& filename);

public slots:
    void import(const QString& filename);

signals:
    void importDone();
    void importProgress(int);

private:
    VoxelTree _voxelTree;
    ImportTask* _task;
    
    void cleanupTask();

private slots:
    void finishImport();
};

#endif // hifi_VoxelImporter_h
