//
//  ParticleTreeHeadlessViewer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/26/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
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
