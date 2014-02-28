//
//  ClipboardScriptingInterface.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable interface for the Application clipboard
//

#ifndef __interface__Clipboard__
#define __interface__Clipboard__

#include <QObject>
#include <VoxelDetail.h>

class ClipboardScriptingInterface : public QObject {
    Q_OBJECT
public:
    ClipboardScriptingInterface();

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

    void importVoxels();

    void nudgeVoxel(const VoxelDetail& sourceVoxel, const glm::vec3& nudgeVec);
    void nudgeVoxel(float x, float y, float z, float s, const glm::vec3& nudgeVec);
    
    void copyTo(float x, float y, float z, float s, const QString source, const QString destination);
    void pasteFrom(float x, float y, float z, float s, const QString source, const QString destination);
};

#endif // __interface__Clipboard__
