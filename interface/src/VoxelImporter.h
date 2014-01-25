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

class VoxelImporter : public QObject {
    Q_OBJECT
public:
    VoxelImporter(QWidget* parent = NULL);
    ~VoxelImporter();

    void init(QSettings* settings);
    void reset();
    void saveSettings(QSettings* settings);

    VoxelTree* getVoxelTree() { return &_voxelTree; }

public slots:
    int exec();
    int preImport();
    int import();

private slots:
    void launchTask();

private:
    VoxelTree _voxelTree;
    ImportDialog _importDialog;

    QString _filename;

    ImportTask* _currentTask;
    ImportTask* _nextTask;
};

#endif /* defined(__hifi__VoxelImporter__) */
