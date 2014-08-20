//
//  RenderableSphereEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
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
    qDebug() << "RenderableSphereEntityItem::factory(const EntityItemID& entityItemID, const EntityItemProperties& properties)...";
    return new RenderableSphereEntityItem(entityID, properties);
}

void RenderableSphereEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableSphereEntityItem::render");
    assert(getType() == EntityTypes::Sphere);
    glm::vec3 position = getPosition() * (float)TREE_SCALE;
    float radius = getRadius() * (float)TREE_SCALE;
    glm::quat rotation = getRotation();


    glColor3ub(getColor()[RED_INDEX], getColor()[GREEN_INDEX], getColor()[BLUE_INDEX]);
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        
        // TODO: we can probably get rid of this rotation until we support different radius dimensions
        //glm::vec3 axis = glm::axis(rotation);
        //glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glutSolidSphere(radius, 15, 15);
    
    glPopMatrix();
};
