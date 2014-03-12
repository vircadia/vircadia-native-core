//
//  LocalVoxelsOverlay.cpp
//  hifi
//
//  Created by Clément Brisset on 2/28/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//
// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QScriptValue>

#include <VoxelSystem.h>
#include <Application.h>

#include "LocalVoxelsOverlay.h"

QMap<QString, WeakVoxelSystemPointer> LocalVoxelsOverlay::_voxelSystemMap;

LocalVoxelsOverlay::LocalVoxelsOverlay() :
    Volume3DOverlay(),
    _voxelCount(0)
{
}

LocalVoxelsOverlay::~LocalVoxelsOverlay() {
    _voxelSystem->changeTree(new VoxelTree());
    _voxelSystem.clear();
    if (_voxelSystemMap.value(_treeName).isNull()) {
        _voxelSystemMap.remove(_treeName);
    }
    _tree.clear();
    LocalVoxelsList::getInstance()->remove(_treeName);
}

void LocalVoxelsOverlay::update(float deltatime) {
    if (!_voxelSystem->isInitialized()) {
        _voxelSystem->init();
    }
    
    _tree->lockForWrite();
    if (_visible && _voxelCount != _tree->getOctreeElementsCount()) {
        _voxelCount = _tree->getOctreeElementsCount();
        _voxelSystem->forceRedrawEntireTree();
    }
    _tree->unlock();
}

void LocalVoxelsOverlay::render() {
    if (_visible && _size > 0 && _voxelSystem && _voxelSystem->isInitialized()) {
        glPushMatrix(); {
            glTranslatef(_position.x, _position.y, _position.z);
            glScalef(_size, _size, _size);
            _voxelSystem->render();
        } glPopMatrix();
    }
}

void LocalVoxelsOverlay::setProperties(const QScriptValue &properties) {
    Volume3DOverlay::setProperties(properties);
    
    if (properties.property("scale").isValid()) {
        setSize(properties.property("scale").toVariant().toFloat());
    }
    
    QScriptValue treeName = properties.property("name");
    if (treeName.isValid()) {
        if ((_treeName = treeName.toString()) == DOMAIN_TREE_NAME) {
            qDebug() << "addOverlay(): Can't create overlay from domain tree";
            return;
        }
        _tree = LocalVoxelsList::getInstance()->getTree(_treeName);
        if (_tree.isNull()) {
            qDebug() << "addOverlay(): Invalid tree name";
            return;
        }
    
        _voxelSystem = _voxelSystemMap[_treeName];
        if (_voxelSystem.isNull()) {
            _voxelSystem = StrongVoxelSystemPointer(new VoxelSystem(1,
                                                                    DEFAULT_MAX_VOXELS_PER_SYSTEM,
                                                                    _tree.data()));
            _voxelSystemMap.insert(_treeName, _voxelSystem);
        }
    }
}

