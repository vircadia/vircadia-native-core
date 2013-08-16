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

class ImportTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    ImportTask(VoxelSystem* voxelSystem, const QString &filename);
    void run();

private:
    VoxelSystem*  _voxelSystem;
    QString       _filename;
};

class VoxelImporter : public QObject {
    Q_OBJECT
public:
    VoxelImporter(QWidget* parent = NULL);

    void init();

public slots:
    void import(const QString &filename = "");
    void preImport(const QString &filename);

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
