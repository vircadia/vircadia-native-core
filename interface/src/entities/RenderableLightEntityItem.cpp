//
//  RenderableLightEntityItem.cpp
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
#include <LightEntityItem.h>


#include "Application.h"
#include "Menu.h"
#include "EntityTreeRenderer.h"
#include "RenderableLightEntityItem.h"


EntityItem* RenderableLightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableLightEntityItem(entityID, properties);
}

void RenderableLightEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLightEntityItem::render");
    assert(getType() == EntityTypes::Light);
    glm::vec3 position = getPositionInMeters();
    glm::vec3 center = getCenterInMeters();
    glm::vec3 dimensions = getDimensions() * (float)TREE_SCALE;
    glm::quat rotation = getRotation();
    float largestDiameter = glm::max(dimensions.x, dimensions.y, dimensions.z);

    const float MAX_COLOR = 255.0f;
    float red = getColor()[RED_INDEX] / MAX_COLOR;
    float green = getColor()[GREEN_INDEX] / MAX_COLOR;
    float blue = getColor()[BLUE_INDEX] / MAX_COLOR;
    float alpha = getLocalRenderAlpha();
    
    /*
    /// Adds a point light to render for the current frame.
    void addPointLight(const glm::vec3& position, float radius, const glm::vec3& ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& diffuse = glm::vec3(1.0f, 1.0f, 1.0f), const glm::vec3& specular = glm::vec3(1.0f, 1.0f, 1.0f),
        float constantAttenuation = 1.0f, float linearAttenuation = 0.0f, float quadraticAttenuation = 0.0f);
        
    /// Adds a spot light to render for the current frame.
    void addSpotLight(const glm::vec3& position, float radius, const glm::vec3& ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& diffuse = glm::vec3(1.0f, 1.0f, 1.0f), const glm::vec3& specular = glm::vec3(1.0f, 1.0f, 1.0f),
        float constantAttenuation = 1.0f, float linearAttenuation = 0.0f, float quadraticAttenuation = 0.0f,
        const glm::vec3& direction = glm::vec3(0.0f, 0.0f, -1.0f), float exponent = 0.0f, float cutoff = PI);
    */
    
    glm::vec3 ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 diffuse = glm::vec3(red, green, blue);
    glm::vec3 specular = glm::vec3(red, green, blue);
    glm::vec3 direction = IDENTITY_FRONT * rotation;
    float constantAttenuation = 1.0f;
    float linearAttenuation = 0.0f; 
    float quadraticAttenuation = 0.0f;

    if (_isSpotlight) {
        Application::getInstance()->getDeferredLightingEffect()->addSpotLight(position, largestDiameter / 2.0f, 
            ambient, diffuse, specular, constantAttenuation, linearAttenuation, quadraticAttenuation,
            direction);
    } else {
        Application::getInstance()->getDeferredLightingEffect()->addPointLight(position, largestDiameter / 2.0f, 
            ambient, diffuse, specular, constantAttenuation, linearAttenuation, quadraticAttenuation);
    }


    bool wantDebug = false;
    if (wantDebug) {
        glColor4f(red, green, blue, alpha);
                    
        glPushMatrix();
            glTranslatef(position.x, position.y, position.z);
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        
        
            glPushMatrix();
                glm::vec3 positionToCenter = center - position;
                glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

                glScalef(dimensions.x, dimensions.y, dimensions.z);
                Application::getInstance()->getDeferredLightingEffect()->renderWireSphere(0.5f, 15, 15);
            glPopMatrix();
        glPopMatrix();
    }
        
};
