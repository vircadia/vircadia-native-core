//
//  ClipboardScriptingInterface.h
//  interface/src/scripting
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ClipboardScriptingInterface_h
#define hifi_ClipboardScriptingInterface_h

#include <QObject>
#include <VoxelDetail.h>

class ClipboardScriptingInterface : public QObject {
    Q_OBJECT
public:
    ClipboardScriptingInterface();

signals:
    void readyToImport();
    
public slots:
    void cutVoxel(const VoxelDetail& sourceVoxel);
    void cutVoxel(float x, float y, float z, float s);

    void copyVoxel(const VoxelDetail& sourceVoxel);
    void copyVoxel(float x, float y, float z, float s);

    void pasteVoxel(const VoxelDetail& destinationVoxel);
    void pasteVoxel(float x, float y, float z, float s);

    void deleteVoxel(const VoxelDetail& sourceVoxel);
    void deleteVoxel(float x, float y, float z, float s);

    void exportVoxel(const VoxelDetail& sourceVoxel);
    void exportVoxel(float x, float y, float z, float s);

    bool importVoxels();
    bool importVoxels(const QString& filename);
    bool importVoxels(const QString& filename, float x, float y, float z, float s);
    bool importVoxels(const QString& filename, const VoxelDetail& destinationVoxel);

    void nudgeVoxel(const VoxelDetail& sourceVoxel, const glm::vec3& nudgeVec);
    void nudgeVoxel(float x, float y, float z, float s, const glm::vec3& nudgeVec);

    bool importModels(const QString& filename);
    bool exportModels(const QString& filename, float x, float y, float z, float s);
    void pasteModels(float x, float y, float z, float s);
};

#endif // hifi_ClipboardScriptingInterface_h
