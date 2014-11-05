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
#include <StreamUtils.h>

#include "Circle3DOverlay.h"
#include "renderer/GlowEffect.h"

Circle3DOverlay::Circle3DOverlay() :
    _startAt(0.0f),
    _endAt(360.0f),
    _outerRadius(1.0f),
    _innerRadius(0.0f),
    _hasTickMarks(false),
    _majorTickMarksAngle(0.0f),
    _minorTickMarksAngle(0.0f),
    _majorTickMarksLength(0.0f),
    _minorTickMarksLength(0.0f)
{
    _majorTickMarksColor.red = _majorTickMarksColor.green = _majorTickMarksColor.blue = (unsigned char)0;
    _minorTickMarksColor.red = _minorTickMarksColor.green = _minorTickMarksColor.blue = (unsigned char)0;
}

Circle3DOverlay::~Circle3DOverlay() {
}

void Circle3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();

    if (alpha == 0.0) {
        return; // do nothing if our alpha is 0, we're not visible
    }
    
    const float FULL_CIRCLE = 360.0f;
    const float SLICES = 180.0f;  // The amount of segment to create the circle
    const float SLICE_ANGLE = FULL_CIRCLE / SLICES;

    //const int slices = 15;
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
                float angleInRadians = glm::radians(angle);
                glm::vec2 firstInnerPoint(cos(angleInRadians) * innerRadius, sin(angleInRadians) * innerRadius);
                glm::vec2 firstOuterPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);

                glVertex2f(firstInnerPoint.x, firstInnerPoint.y);
                glVertex2f(firstOuterPoint.x, firstOuterPoint.y);

                while (angle < endAt) {
                    angleInRadians = glm::radians(angle);
                    glm::vec2 thisInnerPoint(cos(angleInRadians) * innerRadius, sin(angleInRadians) * innerRadius);
                    glm::vec2 thisOuterPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);

                    glVertex2f(thisOuterPoint.x, thisOuterPoint.y);
                    glVertex2f(thisInnerPoint.x, thisInnerPoint.y);
                
                    angle += SLICE_ANGLE;
                }
            
                // get the last slice portion....
                angle = endAt;
                angleInRadians = glm::radians(angle);
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
                float angleInRadians = glm::radians(angle);
                glm::vec2 firstPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                glVertex2f(firstPoint.x, firstPoint.y);

                while (angle < endAt) {
                    angle += SLICE_ANGLE;
                    angleInRadians = glm::radians(angle);
                    glm::vec2 thisPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                    glVertex2f(thisPoint.x, thisPoint.y);

                    if (getIsDashedLine()) {
                        angle += SLICE_ANGLE / 2.0f; // short gap
                        angleInRadians = glm::radians(angle);
                        glm::vec2 dashStartPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                        glVertex2f(dashStartPoint.x, dashStartPoint.y);
                    }
                }
            
                // get the last slice portion....
                angle = endAt;
                angleInRadians = glm::radians(angle);
                glm::vec2 lastOuterPoint(cos(angleInRadians) * outerRadius, sin(angleInRadians) * outerRadius);
                glVertex2f(lastOuterPoint.x, lastOuterPoint.y);
                glEnd();
            }
            
            // draw our tick marks
            // for our overlay, is solid means we draw a ring between the inner and outer radius of the circle, otherwise
            // we just draw a line...
            if (getHasTickMarks()) {
                glBegin(GL_LINES);

                // draw our major tick marks
                if (getMajorTickMarksAngle() > 0.0f && getMajorTickMarksLength() != 0.0f) {
                
                    xColor color = getMajorTickMarksColor();
                    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);
                
                    float tickMarkAngle = getMajorTickMarksAngle();
                    float angle = startAt - fmod(startAt, tickMarkAngle) + tickMarkAngle; 
                    float angleInRadians = glm::radians(angle);
                    float tickMarkLength = getMajorTickMarksLength();
                    float startRadius = (tickMarkLength > 0.0f) ? innerRadius : outerRadius;
                    float endRadius = startRadius + tickMarkLength;

                    while (angle <= endAt) {
                        angleInRadians = glm::radians(angle);

                        glm::vec2 thisPointA(cos(angleInRadians) * startRadius, sin(angleInRadians) * startRadius);
                        glm::vec2 thisPointB(cos(angleInRadians) * endRadius, sin(angleInRadians) * endRadius);

                        glVertex2f(thisPointA.x, thisPointA.y);
                        glVertex2f(thisPointB.x, thisPointB.y);
                
                        angle += tickMarkAngle;
                    }
                }

                // draw our minor tick marks
                if (getMinorTickMarksAngle() > 0.0f && getMinorTickMarksLength() != 0.0f) {
                
                    xColor color = getMinorTickMarksColor();
                    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);
                
                    float tickMarkAngle = getMinorTickMarksAngle();
                    float angle = startAt - fmod(startAt, tickMarkAngle) + tickMarkAngle; 
                    float angleInRadians = glm::radians(angle);
                    float tickMarkLength = getMinorTickMarksLength();
                    float startRadius = (tickMarkLength > 0.0f) ? innerRadius : outerRadius;
                    float endRadius = startRadius + tickMarkLength;

                    while (angle <= endAt) {
                        angleInRadians = glm::radians(angle);

                        glm::vec2 thisPointA(cos(angleInRadians) * startRadius, sin(angleInRadians) * startRadius);
                        glm::vec2 thisPointB(cos(angleInRadians) * endRadius, sin(angleInRadians) * endRadius);

                        glVertex2f(thisPointA.x, thisPointA.y);
                        glVertex2f(thisPointB.x, thisPointB.y);
                
                        angle += tickMarkAngle;
                    }
                }

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

    QScriptValue hasTickMarks = properties.property("hasTickMarks");
    if (hasTickMarks.isValid()) {
        setHasTickMarks(hasTickMarks.toVariant().toBool());
    }

    QScriptValue majorTickMarksAngle = properties.property("majorTickMarksAngle");
    if (majorTickMarksAngle.isValid()) {
        setMajorTickMarksAngle(majorTickMarksAngle.toVariant().toFloat());
    }

    QScriptValue minorTickMarksAngle = properties.property("minorTickMarksAngle");
    if (minorTickMarksAngle.isValid()) {
        setMinorTickMarksAngle(minorTickMarksAngle.toVariant().toFloat());
    }

    QScriptValue majorTickMarksLength = properties.property("majorTickMarksLength");
    if (majorTickMarksLength.isValid()) {
        setMajorTickMarksLength(majorTickMarksLength.toVariant().toFloat());
    }

    QScriptValue minorTickMarksLength = properties.property("minorTickMarksLength");
    if (minorTickMarksLength.isValid()) {
        setMinorTickMarksLength(minorTickMarksLength.toVariant().toFloat());
    }

    QScriptValue majorTickMarksColor = properties.property("majorTickMarksColor");
    if (majorTickMarksColor.isValid()) {
        QScriptValue red = majorTickMarksColor.property("red");
        QScriptValue green = majorTickMarksColor.property("green");
        QScriptValue blue = majorTickMarksColor.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _majorTickMarksColor.red = red.toVariant().toInt();
            _majorTickMarksColor.green = green.toVariant().toInt();
            _majorTickMarksColor.blue = blue.toVariant().toInt();
        }
    }

    QScriptValue minorTickMarksColor = properties.property("minorTickMarksColor");
    if (minorTickMarksColor.isValid()) {
        QScriptValue red = minorTickMarksColor.property("red");
        QScriptValue green = minorTickMarksColor.property("green");
        QScriptValue blue = minorTickMarksColor.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _minorTickMarksColor.red = red.toVariant().toInt();
            _minorTickMarksColor.green = green.toVariant().toInt();
            _minorTickMarksColor.blue = blue.toVariant().toInt();
        }
    }
}

bool Circle3DOverlay::findRayIntersection(const glm::vec3& origin, 
                                const glm::vec3& direction, float& distance, BoxFace& face) const {

    bool intersects = Planar3DOverlay::findRayIntersection(origin, direction, distance, face);
    if (intersects) {
        glm::vec3 hitAt = origin + (direction * distance);
        float distanceToHit = glm::distance(hitAt, _position);

        float maxDimension = glm::max(_dimensions.x, _dimensions.y);
        float innerRadius = maxDimension * getInnerRadius();
        float outerRadius = maxDimension * getOuterRadius();
        
        // TODO: this really only works for circles, we should be handling the ellipse case as well...
        if (distanceToHit < innerRadius || distanceToHit > outerRadius) {
            intersects = false;
        }
    }
    return intersects;
}

Circle3DOverlay* Circle3DOverlay::createClone() {
    Circle3DOverlay* clone = new Circle3DOverlay();
    writeToClone(clone);
    return clone;
}

void Circle3DOverlay::writeToClone(Circle3DOverlay* clone) {
    Planar3DOverlay::writeToClone(clone);
    clone->setStartAt(getStartAt());
    clone->setEndAt(getEndAt());
    clone->setOuterRadius(getOuterRadius());
    clone->setInnerRadius(getInnerRadius());
    clone->setHasTickMarks(getHasTickMarks());
    clone->setMajorTickMarksAngle(getMajorTickMarksAngle());
    clone->setMinorTickMarksAngle(getMinorTickMarksAngle());
    clone->setMajorTickMarksLength(getMajorTickMarksLength());
    clone->setMinorTickMarksLength(getMinorTickMarksLength());
    clone->setMajorTickMarksColor(getMajorTickMarksColor());
    clone->setMinorTickMarksColor(getMinorTickMarksColor());
}
