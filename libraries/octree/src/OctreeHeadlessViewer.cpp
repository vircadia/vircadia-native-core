//
//  OctreeHeadlessViewer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/26/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#include "OctreeHeadlessViewer.h"

OctreeHeadlessViewer::OctreeHeadlessViewer() :
    OctreeRenderer() {
}

OctreeHeadlessViewer::~OctreeHeadlessViewer() {
}

void OctreeHeadlessViewer::init() {
    OctreeRenderer::init();
    setViewFrustum(&_viewFrustum);
}

