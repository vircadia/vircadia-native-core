//
//  LocalVoxelsOverlay.cpp
//  hifi
//
//  Created by Clément Brisset on 2/28/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#include <LocalVoxelsList.h>
#include <VoxelSystem.h>

#include "LocalVoxelsOverlay.h"

struct OverlayElement {
    QString treeName;
    StrongVoxelTreePointer tree; // so that the tree doesn't get freed
    glm::vec3 position;
    float scale;
    bool wantDisplay;
    StrongVoxelSystemPointer voxelSystem;
};


LocalVoxelsOverlay::LocalVoxelsOverlay() {
}

LocalVoxelsOverlay::~LocalVoxelsOverlay() {
}

void LocalVoxelsOverlay::render() {
    QMap<QString, OverlayElement>::iterator i;
    for (i = _overlayMap.begin(); i != _overlayMap.end(); ++i) {
        if (i->wantDisplay && i->scale > 0) {
            glPushMatrix();
                glTranslatef(i->position.x, i->position.y, i->position.z);
                glScalef(i->scale, i->scale, i->scale);
                i->voxelSystem->render();
            glPopMatrix();
        }
    }
}

void LocalVoxelsOverlay::addOverlay(QString overlayName, QString treeName) {
    if (treeName == DOMAIN_TREE_NAME) {
        qDebug() << "addOverlay(): Can't create overlay from domain tree";
        return;
    }
    if (_overlayMap.contains(overlayName)) {
        qDebug() << "addOverlay(): Overlay name elready in use";
        return;
    }
    
    StrongVoxelTreePointer tree = LocalVoxelsList::getInstance()->getTree(treeName);
    if (tree.isNull()) {
        qDebug() << "addOverlay(): Invalid tree name";
        return;
    }
    StrongVoxelSystemPointer voxelSystem = _voxelSystemMap[treeName];
    if (voxelSystem.isNull()) {
        voxelSystem = StrongVoxelSystemPointer(new VoxelSystem(TREE_SCALE,
                                                               DEFAULT_MAX_VOXELS_PER_SYSTEM,
                                                               tree.data()));
        _voxelSystemMap.insert(treeName, voxelSystem);
    }
    
    OverlayElement element = {
        treeName,
        tree,
        glm::vec3(0, 0, 0),
        0,
        false,
        voxelSystem
    };
    _overlayMap.insert(overlayName, element);
}

void LocalVoxelsOverlay::setPosition(QString overlayName, float x, float y, float z) {
    if (_overlayMap.contains(overlayName)) {
        _overlayMap[overlayName].position = glm::vec3(x, y, z);
    }
}

void LocalVoxelsOverlay::setScale(QString overlayName, float scale) {
    if (_overlayMap.contains(overlayName)) {
        _overlayMap[overlayName].scale = scale;
    }
}

void LocalVoxelsOverlay::display(QString overlayName, bool wantDisplay) {
    if (_overlayMap.contains(overlayName)) {
        _overlayMap[overlayName].wantDisplay = wantDisplay;
    }
}

void LocalVoxelsOverlay::removeOverlay(QString overlayName) {
    if (_overlayMap.contains(overlayName)) {
        QString treeName = _overlayMap.take(overlayName).treeName;
        
        if (_voxelSystemMap.value(treeName).isNull()) {
            _voxelSystemMap.remove(treeName);
        }
    }
}







