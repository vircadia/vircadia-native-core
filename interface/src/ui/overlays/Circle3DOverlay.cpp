//
//  Circle3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Circle3DOverlay.h"

#include <DeferredLightingEffect.h>
#include <GeometryCache.h>
#include <RegisteredMetaTypes.h>


QString const Circle3DOverlay::TYPE = "circle3d";

Circle3DOverlay::Circle3DOverlay() :
    _startAt(0.0f),
    _endAt(360.0f),
    _outerRadius(1.0f),
    _innerRadius(0.0f),
    _hasTickMarks(false),
    _majorTickMarksAngle(0.0f),
    _minorTickMarksAngle(0.0f),
    _majorTickMarksLength(0.0f),
    _minorTickMarksLength(0.0f),
    _quadVerticesID(GeometryCache::UNKNOWN_ID),
    _lineVerticesID(GeometryCache::UNKNOWN_ID),
    _majorTicksVerticesID(GeometryCache::UNKNOWN_ID),
    _minorTicksVerticesID(GeometryCache::UNKNOWN_ID),
    _lastStartAt(-1.0f),
    _lastEndAt(-1.0f),
    _lastOuterRadius(-1.0f),
    _lastInnerRadius(-1.0f)
{
    _majorTickMarksColor.red = _majorTickMarksColor.green = _majorTickMarksColor.blue = (unsigned char)0;
    _minorTickMarksColor.red = _minorTickMarksColor.green = _minorTickMarksColor.blue = (unsigned char)0;
}

Circle3DOverlay::Circle3DOverlay(const Circle3DOverlay* circle3DOverlay) :
    Planar3DOverlay(circle3DOverlay),
    _startAt(circle3DOverlay->_startAt),
    _endAt(circle3DOverlay->_endAt),
    _outerRadius(circle3DOverlay->_outerRadius),
    _innerRadius(circle3DOverlay->_innerRadius),
    _hasTickMarks(circle3DOverlay->_hasTickMarks),
    _majorTickMarksAngle(circle3DOverlay->_majorTickMarksAngle),
    _minorTickMarksAngle(circle3DOverlay->_minorTickMarksAngle),
    _majorTickMarksLength(circle3DOverlay->_majorTickMarksLength),
    _minorTickMarksLength(circle3DOverlay->_minorTickMarksLength),
    _majorTickMarksColor(circle3DOverlay->_majorTickMarksColor),
    _minorTickMarksColor(circle3DOverlay->_minorTickMarksColor),
    _quadVerticesID(GeometryCache::UNKNOWN_ID),
    _lineVerticesID(GeometryCache::UNKNOWN_ID),
    _majorTicksVerticesID(GeometryCache::UNKNOWN_ID),
    _minorTicksVerticesID(GeometryCache::UNKNOWN_ID),
    _lastStartAt(-1.0f),
    _lastEndAt(-1.0f),
    _lastOuterRadius(-1.0f),
    _lastInnerRadius(-1.0f)
{
}

void Circle3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();

    if (alpha == 0.0f) {
        return; // do nothing if our alpha is 0, we're not visible
    }

    // Create the circle in the coordinates origin
    float outerRadius = getOuterRadius();
    float innerRadius = getInnerRadius(); // only used in solid case
    float startAt = getStartAt();
    float endAt = getEndAt();

    bool geometryChanged = (startAt != _lastStartAt || endAt != _lastEndAt ||
                                innerRadius != _lastInnerRadius || outerRadius != _lastOuterRadius);


    const float FULL_CIRCLE = 360.0f;
    const float SLICES = 180.0f;  // The amount of segment to create the circle
    const float SLICE_ANGLE = FULL_CIRCLE / SLICES;

    xColor colorX = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 color(colorX.red / MAX_COLOR, colorX.green / MAX_COLOR, colorX.blue / MAX_COLOR, alpha);

    bool colorChanged = colorX.red != _lastColor.red || colorX.green != _lastColor.green || colorX.blue != _lastColor.blue;
    _lastColor = colorX;

    auto geometryCache = DependencyManager::get<GeometryCache>();
    
    Q_ASSERT(args->_batch);
    auto& batch = *args->_batch;

    // FIXME: THe line width of _lineWidth is not supported anymore, we ll need a workaround

    auto transform = _transform;
    transform.postScale(glm::vec3(getDimensions(), 1.0f));
    batch.setModelTransform(transform);
    DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch, false, false);
    
    // for our overlay, is solid means we draw a ring between the inner and outer radius of the circle, otherwise
    // we just draw a line...
    if (getIsSolid()) {
        if (_quadVerticesID == GeometryCache::UNKNOWN_ID) {
            _quadVerticesID = geometryCache->allocateID();
        }
        
        if (geometryChanged || colorChanged) {
            
            QVector<glm::vec2> points;
            
            float angle = startAt;
            float angleInRadians = glm::radians(angle);
            glm::vec2 mostRecentInnerPoint(cosf(angleInRadians) * innerRadius, sinf(angleInRadians) * innerRadius);
            glm::vec2 mostRecentOuterPoint(cosf(angleInRadians) * outerRadius, sinf(angleInRadians) * outerRadius);
            
            while (angle < endAt) {
                angleInRadians = glm::radians(angle);
                glm::vec2 thisInnerPoint(cosf(angleInRadians) * innerRadius, sinf(angleInRadians) * innerRadius);
                glm::vec2 thisOuterPoint(cosf(angleInRadians) * outerRadius, sinf(angleInRadians) * outerRadius);
                
                points << mostRecentInnerPoint << mostRecentOuterPoint << thisOuterPoint; // first triangle
                points << mostRecentInnerPoint << thisInnerPoint << thisOuterPoint; // second triangle
                
                angle += SLICE_ANGLE;

                mostRecentInnerPoint = thisInnerPoint;
                mostRecentOuterPoint = thisOuterPoint;
            }
            
            // get the last slice portion....
            angle = endAt;
            angleInRadians = glm::radians(angle);
            glm::vec2 lastInnerPoint(cosf(angleInRadians) * innerRadius, sinf(angleInRadians) * innerRadius);
            glm::vec2 lastOuterPoint(cosf(angleInRadians) * outerRadius, sinf(angleInRadians) * outerRadius);

            points << mostRecentInnerPoint << mostRecentOuterPoint << lastOuterPoint; // first triangle
            points << mostRecentInnerPoint << lastInnerPoint << lastOuterPoint; // second triangle
            
            geometryCache->updateVertices(_quadVerticesID, points, color);
        }
        
        geometryCache->renderVertices(batch, gpu::TRIANGLES, _quadVerticesID);
        
    } else {
        if (_lineVerticesID == GeometryCache::UNKNOWN_ID) {
            _lineVerticesID = geometryCache->allocateID();
        }
        
        if (geometryChanged || colorChanged) {
            QVector<glm::vec2> points;
            
            float angle = startAt;
            float angleInRadians = glm::radians(angle);
            glm::vec2 firstPoint(cosf(angleInRadians) * outerRadius, sinf(angleInRadians) * outerRadius);
            points << firstPoint;
            
            while (angle < endAt) {
                angle += SLICE_ANGLE;
                angleInRadians = glm::radians(angle);
                glm::vec2 thisPoint(cosf(angleInRadians) * outerRadius, sinf(angleInRadians) * outerRadius);
                points << thisPoint;
                
                if (getIsDashedLine()) {
                    angle += SLICE_ANGLE / 2.0f; // short gap
                    angleInRadians = glm::radians(angle);
                    glm::vec2 dashStartPoint(cosf(angleInRadians) * outerRadius, sinf(angleInRadians) * outerRadius);
                    points << dashStartPoint;
                }
            }
            
            // get the last slice portion....
            angle = endAt;
            angleInRadians = glm::radians(angle);
            glm::vec2 lastPoint(cosf(angleInRadians) * outerRadius, sinf(angleInRadians) * outerRadius);
            points << lastPoint;
            
            geometryCache->updateVertices(_lineVerticesID, points, color);
        }
        
        if (getIsDashedLine()) {
            geometryCache->renderVertices(batch, gpu::LINES, _lineVerticesID);
        } else {
            geometryCache->renderVertices(batch, gpu::LINE_STRIP, _lineVerticesID);
        }
    }
    
    // draw our tick marks
    // for our overlay, is solid means we draw a ring between the inner and outer radius of the circle, otherwise
    // we just draw a line...
    if (getHasTickMarks()) {
        
        if (_majorTicksVerticesID == GeometryCache::UNKNOWN_ID) {
            _majorTicksVerticesID = geometryCache->allocateID();
        }
        if (_minorTicksVerticesID == GeometryCache::UNKNOWN_ID) {
            _minorTicksVerticesID = geometryCache->allocateID();
        }
        
        if (geometryChanged) {
            QVector<glm::vec2> majorPoints;
            QVector<glm::vec2> minorPoints;
            
            // draw our major tick marks
            if (getMajorTickMarksAngle() > 0.0f && getMajorTickMarksLength() != 0.0f) {
                
                float tickMarkAngle = getMajorTickMarksAngle();
                float angle = startAt - fmodf(startAt, tickMarkAngle) + tickMarkAngle;
                float angleInRadians = glm::radians(angle);
                float tickMarkLength = getMajorTickMarksLength();
                float startRadius = (tickMarkLength > 0.0f) ? innerRadius : outerRadius;
                float endRadius = startRadius + tickMarkLength;
                
                while (angle <= endAt) {
                    angleInRadians = glm::radians(angle);
                    
                    glm::vec2 thisPointA(cosf(angleInRadians) * startRadius, sinf(angleInRadians) * startRadius);
                    glm::vec2 thisPointB(cosf(angleInRadians) * endRadius, sinf(angleInRadians) * endRadius);
                    
                    majorPoints << thisPointA << thisPointB;
                    
                    angle += tickMarkAngle;
                }
            }
            
            // draw our minor tick marks
            if (getMinorTickMarksAngle() > 0.0f && getMinorTickMarksLength() != 0.0f) {
                
                float tickMarkAngle = getMinorTickMarksAngle();
                float angle = startAt - fmodf(startAt, tickMarkAngle) + tickMarkAngle;
                float angleInRadians = glm::radians(angle);
                float tickMarkLength = getMinorTickMarksLength();
                float startRadius = (tickMarkLength > 0.0f) ? innerRadius : outerRadius;
                float endRadius = startRadius + tickMarkLength;
                
                while (angle <= endAt) {
                    angleInRadians = glm::radians(angle);
                    
                    glm::vec2 thisPointA(cosf(angleInRadians) * startRadius, sinf(angleInRadians) * startRadius);
                    glm::vec2 thisPointB(cosf(angleInRadians) * endRadius, sinf(angleInRadians) * endRadius);
                    
                    minorPoints << thisPointA << thisPointB;
                    
                    angle += tickMarkAngle;
                }
            }
            
            xColor majorColorX = getMajorTickMarksColor();
            glm::vec4 majorColor(majorColorX.red / MAX_COLOR, majorColorX.green / MAX_COLOR, majorColorX.blue / MAX_COLOR, alpha);
            
            geometryCache->updateVertices(_majorTicksVerticesID, majorPoints, majorColor);
            
            xColor minorColorX = getMinorTickMarksColor();
            glm::vec4 minorColor(minorColorX.red / MAX_COLOR, minorColorX.green / MAX_COLOR, minorColorX.blue / MAX_COLOR, alpha);
            
            geometryCache->updateVertices(_minorTicksVerticesID, minorPoints, minorColor);
        }
        
        geometryCache->renderVertices(batch, gpu::LINES, _majorTicksVerticesID);
        
        geometryCache->renderVertices(batch, gpu::LINES, _minorTicksVerticesID);
    }
    
    if (geometryChanged) {
        _lastStartAt = startAt;
        _lastEndAt = endAt;
        _lastInnerRadius = innerRadius;
        _lastOuterRadius = outerRadius;
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

QScriptValue Circle3DOverlay::getProperty(const QString& property) {
    if (property == "startAt") {
        return _startAt;
    }
    if (property == "endAt") {
        return _endAt;
    }
    if (property == "outerRadius") {
        return _outerRadius;
    }
    if (property == "innerRadius") {
        return _innerRadius;
    }
    if (property == "hasTickMarks") {
        return _hasTickMarks;
    }
    if (property == "majorTickMarksAngle") {
        return _majorTickMarksAngle;
    }
    if (property == "minorTickMarksAngle") {
        return _minorTickMarksAngle;
    }
    if (property == "majorTickMarksLength") {
        return _majorTickMarksLength;
    }
    if (property == "minorTickMarksLength") {
        return _minorTickMarksLength;
    }
    if (property == "majorTickMarksColor") {
        return xColorToScriptValue(_scriptEngine, _majorTickMarksColor);
    }
    if (property == "minorTickMarksColor") {
        return xColorToScriptValue(_scriptEngine, _minorTickMarksColor);
    }

    return Planar3DOverlay::getProperty(property);
}


bool Circle3DOverlay::findRayIntersection(const glm::vec3& origin,
                                const glm::vec3& direction, float& distance, BoxFace& face) {

    bool intersects = Planar3DOverlay::findRayIntersection(origin, direction, distance, face);

    if (intersects) {
        glm::vec3 hitPosition = origin + (distance * direction);
        glm::vec3 localHitPosition = glm::inverse(getRotation()) * (hitPosition - getPosition());
        localHitPosition.y = localHitPosition.y * getDimensions().x / getDimensions().y;  // Scale to make circular

        float distanceToHit = glm::length(localHitPosition);
        float innerRadius = getDimensions().x / 2.0f * _innerRadius;
        float outerRadius = getDimensions().x / 2.0f * _outerRadius;

        intersects = innerRadius <= distanceToHit && distanceToHit <= outerRadius;
    }

    return intersects;
}

Circle3DOverlay* Circle3DOverlay::createClone() const {
    return new Circle3DOverlay(this);
}
