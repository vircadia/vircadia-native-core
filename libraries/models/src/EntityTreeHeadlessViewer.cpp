//
//  ModelTreeHeadlessViewer.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelTreeHeadlessViewer.h"

ModelTreeHeadlessViewer::ModelTreeHeadlessViewer() :
    OctreeHeadlessViewer() {
}

ModelTreeHeadlessViewer::~ModelTreeHeadlessViewer() {
}

void ModelTreeHeadlessViewer::init() {
    OctreeHeadlessViewer::init();
}


void ModelTreeHeadlessViewer::update() {
    if (_tree) {
        ModelTree* tree = static_cast<ModelTree*>(_tree);
        if (tree->tryLockForWrite()) {
            tree->update();
            tree->unlock();
        }
    }
}

void ModelTreeHeadlessViewer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<ModelTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}
