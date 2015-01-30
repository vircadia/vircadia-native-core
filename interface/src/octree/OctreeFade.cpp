//
//  OctreeFade.cpp
//  interface/src/octree
//
//  Created by Brad Hefta-Gaub on 8/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <GlowEffect.h>
#include <GeometryCache.h>
#include <OctreeConstants.h>

#include "Application.h"
#include "OctreeFade.h"

const float OctreeFade::FADE_OUT_START =  0.5f;
const float OctreeFade::FADE_OUT_END   =  0.05f;
const float OctreeFade::FADE_OUT_STEP  =  0.9f;
const float OctreeFade::FADE_IN_START  =  0.05f;
const float OctreeFade::FADE_IN_END    =  0.5f;
const float OctreeFade::FADE_IN_STEP   =  1.1f;
const float OctreeFade::DEFAULT_RED    =  0.5f;
const float OctreeFade::DEFAULT_GREEN  =  0.5f;
const float OctreeFade::DEFAULT_BLUE   =  0.5f;

OctreeFade::OctreeFade(FadeDirection direction, float red, float green, float blue) :
    direction(direction),
    red(red),
    green(green),
    blue(blue)
{
    opacity = (direction == FADE_OUT) ? FADE_OUT_START : FADE_IN_START;
}

void OctreeFade::render() {
    DependencyManager::get<GlowEffect>()->begin();

    glDisable(GL_LIGHTING);
    glPushMatrix();
    glScalef(TREE_SCALE, TREE_SCALE, TREE_SCALE);
    glTranslatef(voxelDetails.x + voxelDetails.s * 0.5f,
                 voxelDetails.y + voxelDetails.s * 0.5f,
                 voxelDetails.z + voxelDetails.s * 0.5f);
    glLineWidth(1.0f);
    DependencyManager::get<GeometryCache>()->renderSolidCube(voxelDetails.s, glm::vec4(red, green, blue, opacity));
    glLineWidth(1.0f);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    
    
    DependencyManager::get<GlowEffect>()->end();
    
    opacity *= (direction == FADE_OUT) ? FADE_OUT_STEP : FADE_IN_STEP;
}

bool OctreeFade::isDone() const {
    if (direction == FADE_OUT) {
        return opacity <= FADE_OUT_END;
    } else {
        return opacity >= FADE_IN_END;
    }
    return true; // unexpected case, assume we're done
}
