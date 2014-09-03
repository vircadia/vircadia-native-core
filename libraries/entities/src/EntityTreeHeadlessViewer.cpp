//
//  EntityTreeHeadlessViewer.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityTreeHeadlessViewer.h"

EntityTreeHeadlessViewer::EntityTreeHeadlessViewer() :
    OctreeHeadlessViewer() {
}

EntityTreeHeadlessViewer::~EntityTreeHeadlessViewer() {
}

void EntityTreeHeadlessViewer::init() {
    OctreeHeadlessViewer::init();
}


void EntityTreeHeadlessViewer::update() {
    if (_tree) {
        EntityTree* tree = static_cast<EntityTree*>(_tree);
        if (tree->tryLockForWrite()) {
            tree->update();
            tree->unlock();
        }
    }
}

void EntityTreeHeadlessViewer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<EntityTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}
