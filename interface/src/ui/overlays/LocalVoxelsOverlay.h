//
//  LocalVoxelsOverlay.h
//  interface/src/ui/overlays
//
//  Created by Clément Brisset on 2/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocalVoxelsOverlay_h
#define hifi_LocalVoxelsOverlay_h

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
    LocalVoxelsOverlay(LocalVoxelsOverlay* localVoxelsOverlay);
    ~LocalVoxelsOverlay();
    
    virtual void update(float deltatime);
    virtual void render(RenderArgs* args);
    
    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);

    virtual LocalVoxelsOverlay* createClone();
private:
    static QMap<QString, WeakVoxelSystemPointer> _voxelSystemMap; // treeName/voxelSystem
    
    QString _treeName;
    StrongVoxelTreePointer _tree; // so that the tree doesn't get freed
    unsigned long _voxelCount;
    StrongVoxelSystemPointer _voxelSystem;
};

#endif // hifi_LocalVoxelsOverlay_h
