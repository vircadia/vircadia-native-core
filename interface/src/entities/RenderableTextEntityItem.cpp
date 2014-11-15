//
//  RenderableTextEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include "InterfaceConfig.h"

#include <TextEntityItem.h>
#include <PerfStat.h>

#include "Menu.h"
#include "EntityTreeRenderer.h"
#include "RenderableTextEntityItem.h"

const int FIXED_FONT_POINT_SIZE = 40;
const float LINE_SCALE_RATIO = 1.2f;

EntityItem* RenderableTextEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableTextEntityItem(entityID, properties);
}

void RenderableTextEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableTextEntityItem::render");
    assert(getType() == EntityTypes::Text);
    glm::vec3 position = getPositionInMeters();
    glm::vec3 dimensions = getDimensions() * (float)TREE_SCALE;
    glm::vec3 halfDimensions = dimensions / 2.0f;
    glm::quat rotation = getRotation();
    float leftMargin = 0.1f;
    float rightMargin = 0.1f;
    float topMargin = 0.1f;
    float bottomMargin = 0.1f;

    //qDebug() << "RenderableTextEntityItem::render() id:" << getEntityItemID() << "text:" << getText();

    glPushMatrix(); 
    {
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        const float MAX_COLOR = 255.0f;
        xColor backgroundColor = getBackgroundColorX();
        float alpha = 1.0f; //getBackgroundAlpha();
        glColor4f(backgroundColor.red / MAX_COLOR, backgroundColor.green / MAX_COLOR, backgroundColor.blue / MAX_COLOR, alpha);
       
        const float SLIGHTLY_BEHIND = -0.005f;

        glBegin(GL_QUADS);
            glVertex3f(-halfDimensions.x, -halfDimensions.y, SLIGHTLY_BEHIND);
            glVertex3f(halfDimensions.x, -halfDimensions.y, SLIGHTLY_BEHIND);
            glVertex3f(halfDimensions.x, halfDimensions.y, SLIGHTLY_BEHIND);
            glVertex3f(-halfDimensions.x, halfDimensions.y, SLIGHTLY_BEHIND);
        glEnd();
        
        const int FIXED_FONT_SCALING_RATIO = FIXED_FONT_POINT_SIZE * 40.0f; // this is a ratio determined through experimentation
        
        // Same font properties as textWidth()
        TextRenderer* textRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE);
        float maxHeight = (float)textRenderer->calculateHeight("Xy") * LINE_SCALE_RATIO;
        
        float scaleFactor =  (maxHeight / FIXED_FONT_SCALING_RATIO) * _lineHeight; 

        glTranslatef(-(halfDimensions.x - leftMargin), halfDimensions.y - topMargin, 0.0f);

        glm::vec2 clipMinimum(0.0f, 0.0f);
        glm::vec2 clipDimensions((dimensions.x - (leftMargin + rightMargin)) / scaleFactor, 
                                 (dimensions.y - (topMargin + bottomMargin)) / scaleFactor);

        glScalef(scaleFactor, -scaleFactor, 1.0);
        enableClipPlane(GL_CLIP_PLANE0, -1.0f, 0.0f, 0.0f, clipMinimum.x + clipDimensions.x);
        enableClipPlane(GL_CLIP_PLANE1, 1.0f, 0.0f, 0.0f, -clipMinimum.x);
        enableClipPlane(GL_CLIP_PLANE2, 0.0f, -1.0f, 0.0f, clipMinimum.y + clipDimensions.y);
        enableClipPlane(GL_CLIP_PLANE3, 0.0f, 1.0f, 0.0f, -clipMinimum.y);
    
        xColor textColor = getTextColorX();
        glColor3f(textColor.red / MAX_COLOR, textColor.green / MAX_COLOR, textColor.blue / MAX_COLOR);
        QStringList lines = _text.split("\n");
        int lineOffset = maxHeight;
        float textAlpha = 1.0f; // getTextAlpha()
        foreach(QString thisLine, lines) {
            textRenderer->draw(0, lineOffset, qPrintable(thisLine), textAlpha);
            lineOffset += maxHeight;
        }

        glDisable(GL_CLIP_PLANE0);
        glDisable(GL_CLIP_PLANE1);
        glDisable(GL_CLIP_PLANE2);
        glDisable(GL_CLIP_PLANE3);
        
    } 
    glPopMatrix();
}

void RenderableTextEntityItem::enableClipPlane(GLenum plane, float x, float y, float z, float w) {
    GLdouble coefficients[] = { x, y, z, w };
    glClipPlane(plane, coefficients);
    glEnable(plane);
}



