//
//  ClipboardOverlay.cpp
//  hifi
//
//  Created by Clément Brisset on 2/20/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "ClipboardOverlay.h"
#include "../Application.h"

static int lastVoxelCount = 0;

ClipboardOverlay::ClipboardOverlay() {
}

ClipboardOverlay::~ClipboardOverlay() {
}

void ClipboardOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }
    
    VoxelSystem* voxelSystem = Application::getInstance()->getSharedVoxelSystem();
    VoxelTree* clipboard = Application::getInstance()->getClipboard();
    if (voxelSystem->getTree() != clipboard) {
        voxelSystem->changeTree(clipboard);
    }
    
    glPushMatrix();
    glTranslatef(_position.x, _position.y, _position.z);
    glScalef(_size, _size, _size);
    
    // We only force the redraw when the clipboard content has changed
    if (lastVoxelCount != clipboard->getOctreeElementsCount()) {
        voxelSystem->forceRedrawEntireTree();
        lastVoxelCount = clipboard->getOctreeElementsCount();
    }
    
    voxelSystem->render();
    
    glPopMatrix();
}