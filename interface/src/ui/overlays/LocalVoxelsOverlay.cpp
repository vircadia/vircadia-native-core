//
//  LocalVoxelsOverlay.cpp
//  interface/src/ui/overlays
//
//  Created by Clément Brisset on 2/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QScriptValue>

#include <Application.h>

#include "LocalVoxelsOverlay.h"
#include "voxels/VoxelSystem.h"

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
    
    _tree->lockForRead();
    if (_visible && _voxelCount != _tree->getOctreeElementsCount()) {
        _voxelCount = _tree->getOctreeElementsCount();
        _voxelSystem->forceRedrawEntireTree();
    }
    _tree->unlock();
}

void LocalVoxelsOverlay::render(RenderArgs* args) {
    glm::vec3 dimensions = getDimensions();
    float size = glm::length(dimensions);
    if (_visible && size > 0 && _voxelSystem && _voxelSystem->isInitialized()) {

        float glowLevel = getGlowLevel();
        Glower* glower = NULL;
        if (glowLevel > 0.0f) {
            glower = new Glower(glowLevel);
        }

        glPushMatrix(); {
            glTranslatef(_position.x, _position.y, _position.z);
            glScalef(dimensions.x, dimensions.y, dimensions.z);
            _voxelSystem->render();
        } glPopMatrix();

        if (glower) {
            delete glower;
        }
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

LocalVoxelsOverlay* LocalVoxelsOverlay::createClone() {
    LocalVoxelsOverlay* clone = new LocalVoxelsOverlay();
    writeToClone(clone);
    return clone;
}

QScriptValue LocalVoxelsOverlay::getProperty(const QString& property) {
    if (property == "scale") {
        return vec3toScriptValue(_scriptEngine, getDimensions());
    }
    if (property == "name") {
        return _treeName;
    }

    return Volume3DOverlay::getProperty(property);
}

