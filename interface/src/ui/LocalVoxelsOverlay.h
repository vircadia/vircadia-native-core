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

#include "Volume3DOverlay.h"

#include <QMap>
#include <QSharedPointer>
#include <QWeakPointer>

struct OverlayElement;
typedef QSharedPointer<VoxelSystem> StrongVoxelSystemPointer;
typedef QWeakPointer<VoxelSystem> WeakVoxelSystemPointer;

class LocalVoxelsOverlay : public Volume3DOverlay {
    Q_OBJECT
public:
    LocalVoxelsOverlay();
    ~LocalVoxelsOverlay();
    
    virtual void render();
    
    void addOverlay(QString overlayName, QString treeName);
    void setPosition(QString overlayName, float x, float y, float z);
    void setScale(QString overlayName, float scale);
    void update(QString overlayName);
    void display(QString overlayName, bool wantDisplay);
    void removeOverlay(QString overlayName);
    
    
private:
    QMap<QString, OverlayElement> _overlayMap; // overlayName/overlayElement
    QMap<QString, WeakVoxelSystemPointer> _voxelSystemMap; // treeName/voxelSystem
};

#endif /* defined(__hifi__LocalVoxelsOverlay__) */
