//
//  RenderableSphereEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <FBXReader.h>

#include "InterfaceConfig.h"

#include <PerfStat.h>
#include <SphereEntityItem.h>


#include "Menu.h"
#include "EntityTreeRenderer.h"
#include "RenderableSphereEntityItem.h"


EntityItem* RenderableSphereEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableSphereEntityItem(entityID, properties);
}

void RenderableSphereEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableSphereEntityItem::render");
    assert(getType() == EntityTypes::Sphere);
    glm::vec3 position = getPositionInMeters();
    glm::vec3 center = getCenterInMeters();
    glm::vec3 dimensions = getDimensions() * (float)TREE_SCALE;
    glm::quat rotation = getRotation();

    const float MAX_COLOR = 255;
    glColor4f(getColor()[RED_INDEX] / MAX_COLOR, getColor()[GREEN_INDEX] / MAX_COLOR, 
                    getColor()[BLUE_INDEX] / MAX_COLOR, getLocalRenderAlpha());
                    
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        
        
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

            glScalef(dimensions.x, dimensions.y, dimensions.z);
            Application::getInstance()->getDeferredLightingEffect()->renderSolidSphere(0.5f, 15, 15);
        glPopMatrix();
    glPopMatrix();
};
