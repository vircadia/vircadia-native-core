//
//  LocalVoxelsOverlay.h
//  hifi
//
//  Created by Clément Brisset on 2/28/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable interface for LocalVoxels
//

#ifndef __hifi__LocalVoxelsOverlay__
#define __hifi__LocalVoxelsOverlay__

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QScriptValue>
#include <QMap>
#include <QSharedPointer>
#include <QWeakPointer>

#include <LocalVoxelsList.h>

#include "Volume3DOverlay.h"

typedef QSharedPointer<VoxelSystem> StrongVoxelSystemPointer;
typedef QWeakPointer<VoxelSystem> WeakVoxelSystemPointer;

class LocalVoxelsOverlay : public Volume3DOverlay {
    Q_OBJECT
public:
    LocalVoxelsOverlay();
    ~LocalVoxelsOverlay();
    
    virtual void update(float deltatime);
    virtual void render();
    
    virtual void setProperties(const QScriptValue& properties);
    
private:
    static QMap<QString, WeakVoxelSystemPointer> _voxelSystemMap; // treeName/voxelSystem
    
    QString _treeName;
    StrongVoxelTreePointer _tree; // so that the tree doesn't get freed
    int _voxelCount;
    StrongVoxelSystemPointer _voxelSystem;
};

#endif /* defined(__hifi__LocalVoxelsOverlay__) */
