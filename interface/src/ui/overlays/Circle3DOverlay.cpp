//
//  Circle3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "Circle3DOverlay.h"
#include "renderer/GlowEffect.h"

Circle3DOverlay::Circle3DOverlay() :
    _startAt(0.0f),
    _endAt(360.0f),
    _outerRadius(1.0f),
    _innerRadius(0.0f)
{
}

Circle3DOverlay::~Circle3DOverlay() {
}

void Circle3DOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }
    
    const float PI_OVER_180 = 3.14159265358979f / 180.0f;
    const float FULL_CIRCLE = 360.0f;
    const float SLICES = 180.0f;  // The amount of segment to create the circle
    const float SLICE_ANGLE = FULL_CIRCLE / SLICES;

    //const int slices = 15;
    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);


    glDisable(GL_LIGHTING);
    
    glm::vec3 position = getPosition();
    glm::vec3 center = getCenter();
    glm::vec2 dimensions = getDimensions();
    glm::quat rotation = getRotation();

    float glowLevel = getGlowLevel();
    Glower* glower = NULL;
    if (glowLevel > 0.0f) {
        glower = new Glower(glowLevel);
    }

    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
            glScalef(dimensions.x, dimensions.y, 1.0f);

            // Create the circle in the coordinates origin
            float outerRadius = getOuterRadius(); 
            float innerRadius = getInnerRadius(); // only used in solid case
            float startAt = getStartAt();
            float endAt = getEndAt();

            glLineWidth(_lineWidth);
            
            // for our overlay, is solid means we draw a ring between the inner and outer radius of the circle, otherwise
            // we just draw a line...
            if (getIsSolid()) {
                glBegin(GL_QUAD_STRIP);

                float angle = startAt;
                float angleInRadians = angle * PI_OVER_180;
                glm::vec2 firstInnerPoint(cos(angleInRadians) * innerRadius, sin(angleInRadians) * innerRadius);
                glm::vec2 firstOuterPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);

                glVertex2f(firstInnerPoint.x, firstInnerPoint.y);
                glVertex2f(firstOuterPoint.x, firstOuterPoint.y);

                while (angle < endAt) {
                    angleInRadians = angle * PI_OVER_180;
                    glm::vec2 thisInnerPoint(cos(angleInRadians) * innerRadius, sin(angleInRadians) * innerRadius);
                    glm::vec2 thisOuterPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);

                    glVertex2f(thisOuterPoint.x, thisOuterPoint.y);
                    glVertex2f(thisInnerPoint.x, thisInnerPoint.y);
                
                    angle += SLICE_ANGLE;
                }
            
                // get the last slice portion....
                angle = endAt;
                angleInRadians = angle * PI_OVER_180;
                glm::vec2 lastInnerPoint(cos(angleInRadians) * innerRadius, sin(angleInRadians) * innerRadius);
                glm::vec2 lastOuterPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
            
                glVertex2f(lastOuterPoint.x, lastOuterPoint.y);
                glVertex2f(lastInnerPoint.x, lastInnerPoint.y);

                glEnd();
            } else {
                if (getIsDashedLine()) {
                    glBegin(GL_LINES);
                } else {
                    glBegin(GL_LINE_STRIP);
                }
                

                float angle = startAt;
                float angleInRadians = angle * PI_OVER_180;
                glm::vec2 firstPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                glVertex2f(firstPoint.x, firstPoint.y);

                while (angle < endAt) {
                    angle += SLICE_ANGLE;
                    angleInRadians = angle * PI_OVER_180;
                    glm::vec2 thisPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                    glVertex2f(thisPoint.x, thisPoint.y);

                    if (getIsDashedLine()) {
                        angle += SLICE_ANGLE / 2.0f; // short gap
                        angleInRadians = angle * PI_OVER_180;
                        glm::vec2 dashStartPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                        glVertex2f(dashStartPoint.x, dashStartPoint.y);
                    }
                }
            
                // get the last slice portion....
                angle = endAt;
                angleInRadians = angle * PI_OVER_180;
                glm::vec2 lastOuterPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                glVertex2f(lastOuterPoint.x, lastOuterPoint.y);
                glEnd();
            }
 
        glPopMatrix();
    glPopMatrix();
    
    if (glower) {
        delete glower;
    }
}

void Circle3DOverlay::setProperties(const QScriptValue &properties) {
    Planar3DOverlay::setProperties(properties);
    
    QScriptValue startAt = properties.property("startAt");
    if (startAt.isValid()) {
        setStartAt(startAt.toVariant().toFloat());
    }

    QScriptValue endAt = properties.property("endAt");
    if (endAt.isValid()) {
        setEndAt(endAt.toVariant().toFloat());
    }

    QScriptValue outerRadius = properties.property("outerRadius");
    if (outerRadius.isValid()) {
        setOuterRadius(outerRadius.toVariant().toFloat());
    }

    QScriptValue innerRadius = properties.property("innerRadius");
    if (innerRadius.isValid()) {
        setInnerRadius(innerRadius.toVariant().toFloat());
    }
}








