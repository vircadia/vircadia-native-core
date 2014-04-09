//
//  ParticleTreeHeadlessViewer.cpp
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ParticleTreeHeadlessViewer.h"

ParticleTreeHeadlessViewer::ParticleTreeHeadlessViewer() :
    OctreeHeadlessViewer() {
}

ParticleTreeHeadlessViewer::~ParticleTreeHeadlessViewer() {
}

void ParticleTreeHeadlessViewer::init() {
    OctreeHeadlessViewer::init();
}


void ParticleTreeHeadlessViewer::update() {
    if (_tree) {
        ParticleTree* tree = static_cast<ParticleTree*>(_tree);
        if (tree->tryLockForWrite()) {
            tree->update();
            tree->unlock();
        }
    }
}

void ParticleTreeHeadlessViewer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<ParticleTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}
