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

#include <GeometryUtil.h>
#include <GeometryCache.h>
#include <RegisteredMetaTypes.h>

QString const Circle3DOverlay::TYPE = "circle3d";

Circle3DOverlay::Circle3DOverlay() {
    memset(&_minorTickMarksColor, 0, sizeof(_minorTickMarksColor));
    memset(&_majorTickMarksColor, 0, sizeof(_majorTickMarksColor));
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
    _minorTicksVerticesID(GeometryCache::UNKNOWN_ID)
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

    bool geometryChanged = _dirty;
    _dirty = false;

    const float FULL_CIRCLE = 360.0f;
    const float SLICES = 180.0f;  // The amount of segment to create the circle
    const float SLICE_ANGLE = FULL_CIRCLE / SLICES;
    const float MAX_COLOR = 255.0f;

    auto geometryCache = DependencyManager::get<GeometryCache>();

    Q_ASSERT(args->_batch);
    auto& batch = *args->_batch;
    if (args->_pipeline) {
        batch.setPipeline(args->_pipeline->pipeline);
    }

    // FIXME: THe line width of _lineWidth is not supported anymore, we ll need a workaround

    auto transform = getTransform();
    transform.postScale(glm::vec3(getDimensions(), 1.0f));
    batch.setModelTransform(transform);

    // for our overlay, is solid means we draw a ring between the inner and outer radius of the circle, otherwise
    // we just draw a line...
    if (getIsSolid()) {
        if (!_quadVerticesID) {
            _quadVerticesID = geometryCache->allocateID();
        }

        if (geometryChanged) {
            QVector<glm::vec2> points;
            QVector<glm::vec4> colors;

            float pulseLevel = updatePulse();
            vec4 pulseModifier = vec4(1);
            if (_alphaPulse != 0.0f) {
                pulseModifier.a = (_alphaPulse >= 0.0f) ? pulseLevel : (1.0f - pulseLevel);
            }
            if (_colorPulse != 0.0f) {
                float pulseValue = (_colorPulse >= 0.0f) ? pulseLevel : (1.0f - pulseLevel);
                pulseModifier = vec4(vec3(pulseValue), pulseModifier.a);
            }
            vec4 innerStartColor = vec4(toGlm(_innerStartColor), _innerStartAlpha) * pulseModifier;
            vec4 outerStartColor = vec4(toGlm(_outerStartColor), _outerStartAlpha) * pulseModifier;
            vec4 innerEndColor = vec4(toGlm(_innerEndColor), _innerEndAlpha) * pulseModifier;
            vec4 outerEndColor = vec4(toGlm(_outerEndColor), _outerEndAlpha) * pulseModifier;

            if (_innerRadius <= 0) {
                _solidPrimitive = gpu::TRIANGLE_FAN;
                points << vec2();
                colors << innerStartColor;
                for (float angle = _startAt; angle <= _endAt; angle += SLICE_ANGLE) {
                    float range = (angle - _startAt) / (_endAt - _startAt);
                    float angleRadians = glm::radians(angle);
                    points << glm::vec2(cosf(angleRadians) * _outerRadius, sinf(angleRadians) * _outerRadius);
                    colors << glm::mix(outerStartColor, outerEndColor, range);
                }
            } else {
                _solidPrimitive = gpu::TRIANGLE_STRIP;
                for (float angle = _startAt; angle <= _endAt; angle += SLICE_ANGLE) {
                    float range = (angle - _startAt) / (_endAt - _startAt);

                    float angleRadians = glm::radians(angle);
                    points << glm::vec2(cosf(angleRadians) * _innerRadius, sinf(angleRadians) * _innerRadius);
                    colors << glm::mix(innerStartColor, innerEndColor, range);

                    points << glm::vec2(cosf(angleRadians) * _outerRadius, sinf(angleRadians) * _outerRadius);
                    colors << glm::mix(outerStartColor, outerEndColor, range);
                }
            }
            geometryCache->updateVertices(_quadVerticesID, points, colors);
        }
        
        geometryCache->renderVertices(batch, _solidPrimitive, _quadVerticesID);
        
    } else {
        if (!_lineVerticesID) {
            _lineVerticesID = geometryCache->allocateID();
        }
        
        if (geometryChanged) {
            QVector<glm::vec2> points;
            
            float angle = _startAt;
            float angleInRadians = glm::radians(angle);
            glm::vec2 firstPoint(cosf(angleInRadians) * _outerRadius, sinf(angleInRadians) * _outerRadius);
            points << firstPoint;
            
            while (angle < _endAt) {
                angle += SLICE_ANGLE;
                angleInRadians = glm::radians(angle);
                glm::vec2 thisPoint(cosf(angleInRadians) * _outerRadius, sinf(angleInRadians) * _outerRadius);
                points << thisPoint;
                
                if (getIsDashedLine()) {
                    angle += SLICE_ANGLE / 2.0f; // short gap
                    angleInRadians = glm::radians(angle);
                    glm::vec2 dashStartPoint(cosf(angleInRadians) * _outerRadius, sinf(angleInRadians) * _outerRadius);
                    points << dashStartPoint;
                }
            }
            
            // get the last slice portion....
            angle = _endAt;
            angleInRadians = glm::radians(angle);
            glm::vec2 lastPoint(cosf(angleInRadians) * _outerRadius, sinf(angleInRadians) * _outerRadius);
            points << lastPoint;
            geometryCache->updateVertices(_lineVerticesID, points, vec4(toGlm(getColor()), getAlpha()));
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
                float angle = _startAt - fmodf(_startAt, tickMarkAngle) + tickMarkAngle;
                float angleInRadians = glm::radians(angle);
                float tickMarkLength = getMajorTickMarksLength();
                float startRadius = (tickMarkLength > 0.0f) ? _innerRadius : _outerRadius;
                float endRadius = startRadius + tickMarkLength;
                
                while (angle <= _endAt) {
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
                float angle = _startAt - fmodf(_startAt, tickMarkAngle) + tickMarkAngle;
                float angleInRadians = glm::radians(angle);
                float tickMarkLength = getMinorTickMarksLength();
                float startRadius = (tickMarkLength > 0.0f) ? _innerRadius : _outerRadius;
                float endRadius = startRadius + tickMarkLength;
                
                while (angle <= _endAt) {
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
}

const render::ShapeKey Circle3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace().withUnlit();
    if (getAlpha() != 1.0f) {
        builder.withTranslucent();
    }
    if (!getIsSolid()) {
        builder.withUnlit().withDepthBias();
    }
    return builder.build();
}

template<typename T> T fromVariant(const QVariant& v, bool& valid) {
    valid = v.isValid();
    return qvariant_cast<T>(v);
}

template<> xColor fromVariant(const QVariant& v, bool& valid) {
    return xColorFromVariant(v, valid);
}

template<typename T>
bool updateIfValid(const QVariantMap& properties, const char* key, T& output) {
    bool valid;
    T result = fromVariant<T>(properties[key], valid);
    if (!valid) {
        return false;
    }

    // Don't signal updates if the value was already set
    if (result == output) {
        return false;
    }

    output = result;
    return true;
}

// Multicast, many outputs
template<typename T>
bool updateIfValid(const QVariantMap& properties, const char* key, std::initializer_list<std::reference_wrapper<T>> outputs) {
    bool valid;
    T value = fromVariant<T>(properties[key], valid);
    if (!valid) {
        return false;
    }
    bool updated = false;
    for (T& output : outputs) {
        if (output != value) {
            output = value;
            updated = true;
        }
    }
    return updated;
}

// Multicast, multiple possible inputs, in order of preference
template<typename T>
bool updateIfValid(const QVariantMap& properties, const std::initializer_list<const char*> keys, T& output) {
    for (const char* key : keys) {
        if (updateIfValid<T>(properties, key, output)) {
            return true;
        }
    }
    return false;
}


void Circle3DOverlay::setProperties(const QVariantMap& properties) {
    Planar3DOverlay::setProperties(properties);
    _dirty |= updateIfValid<float>(properties, "alpha", { _innerStartAlpha, _innerEndAlpha, _outerStartAlpha, _outerEndAlpha });
    _dirty |= updateIfValid<float>(properties, "Alpha", { _innerStartAlpha, _innerEndAlpha, _outerStartAlpha, _outerEndAlpha });
    _dirty |= updateIfValid<float>(properties, "startAlpha", { _innerStartAlpha, _outerStartAlpha });
    _dirty |= updateIfValid<float>(properties, "endAlpha", { _innerEndAlpha, _outerEndAlpha });
    _dirty |= updateIfValid<float>(properties, "innerAlpha", { _innerStartAlpha, _innerEndAlpha });
    _dirty |= updateIfValid<float>(properties, "outerAlpha", { _outerStartAlpha, _outerEndAlpha });
    _dirty |= updateIfValid(properties, "innerStartAlpha", _innerStartAlpha);
    _dirty |= updateIfValid(properties, "innerEndAlpha", _innerEndAlpha);
    _dirty |= updateIfValid(properties, "outerStartAlpha", _outerStartAlpha);
    _dirty |= updateIfValid(properties, "outerEndAlpha", _outerEndAlpha);

    _dirty |= updateIfValid<xColor>(properties, "color", { _innerStartColor, _innerEndColor, _outerStartColor, _outerEndColor });
    _dirty |= updateIfValid<xColor>(properties, "startColor", { _innerStartColor, _outerStartColor } );
    _dirty |= updateIfValid<xColor>(properties, "endColor", { _innerEndColor, _outerEndColor } );
    _dirty |= updateIfValid<xColor>(properties, "innerColor", { _innerStartColor, _innerEndColor } );
    _dirty |= updateIfValid<xColor>(properties, "outerColor", { _outerStartColor, _outerEndColor } );
    _dirty |= updateIfValid(properties, "innerStartColor", _innerStartColor);
    _dirty |= updateIfValid(properties, "innerEndColor", _innerEndColor);
    _dirty |= updateIfValid(properties, "outerStartColor", _outerStartColor);
    _dirty |= updateIfValid(properties, "outerEndColor", _outerEndColor);

    _dirty |= updateIfValid(properties, "startAt", _startAt);
    _dirty |= updateIfValid(properties, "endAt", _endAt);

    _dirty |= updateIfValid(properties, { "radius", "outerRadius" }, _outerRadius);
    _dirty |= updateIfValid(properties, "innerRadius", _innerRadius);
    _dirty |= updateIfValid(properties, "hasTickMarks", _hasTickMarks);
    _dirty |= updateIfValid(properties, "majorTickMarksAngle", _majorTickMarksAngle);
    _dirty |= updateIfValid(properties, "minorTickMarksAngle", _minorTickMarksAngle);
    _dirty |= updateIfValid(properties, "majorTickMarksLength", _majorTickMarksLength);
    _dirty |= updateIfValid(properties, "minorTickMarksLength", _minorTickMarksLength);
    _dirty |= updateIfValid(properties, "majorTickMarksColor", _majorTickMarksColor);
    _dirty |= updateIfValid(properties, "minorTickMarksColor", _minorTickMarksColor);

    if (_innerStartAlpha < 1.0f || _innerEndAlpha < 1.0f || _outerStartAlpha < 1.0f || _outerEndAlpha < 1.0f) {
        // Force the alpha to 0.5, since we'll ignore it in the presence of these other values, but we need
        // it to be non-1 in order to get the right pipeline and non-0 in order to render at all.
        _alpha = 0.5f;
    }
}

QVariant Circle3DOverlay::getProperty(const QString& property) {
    if (property == "startAt") {
        return _startAt;
    }
    if (property == "endAt") {
        return _endAt;
    }
    if (property == "radius") {
        return _outerRadius;
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
        return xColorToVariant(_majorTickMarksColor);
    }
    if (property == "minorTickMarksColor") {
        return xColorToVariant(_minorTickMarksColor);
    }

    return Planar3DOverlay::getProperty(property);
}


bool Circle3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                            BoxFace& face, glm::vec3& surfaceNormal) {

    // Scale the dimensions by the diameter
    glm::vec2 dimensions = getOuterRadius() * 2.0f * getDimensions();
    bool intersects = findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), dimensions, distance);

    if (intersects) {
        glm::vec3 hitPosition = origin + (distance * direction);
        glm::vec3 localHitPosition = glm::inverse(getRotation()) * (hitPosition - getPosition());
        localHitPosition.x /= getDimensions().x;
        localHitPosition.y /= getDimensions().y;
        float distanceToHit = glm::length(localHitPosition);

        intersects = getInnerRadius() <= distanceToHit && distanceToHit <= getOuterRadius();
    }

    return intersects;
}

Circle3DOverlay* Circle3DOverlay::createClone() const {
    return new Circle3DOverlay(this);
}
