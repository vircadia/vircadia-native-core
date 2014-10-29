//
//  AACubeShape.h
//  libraries/physics/src
//
//  Created by Andrew Meadows on 2014.08.22
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AACubeShape_h
#define hifi_AACubeShape_h

#include <QDebug>
#include "Shape.h"

class AACubeShape : public Shape {
public:
    AACubeShape() : Shape(AACUBE_SHAPE), _scale(1.0f) { }
    AACubeShape(float scale) : Shape(AACUBE_SHAPE), _scale(scale) { }
    AACubeShape(float scale, const glm::vec3& position) : Shape(AACUBE_SHAPE, position), _scale(scale) { }

    virtual ~AACubeShape() { }

    float getScale() const { return _scale; }
    void setScale(float scale) { _scale = scale; }

    bool findRayIntersection(RayIntersectionInfo& intersection) const;

    float getVolume() const { return _scale * _scale * _scale; }
    virtual QDebug& dumpToDebug(QDebug& debugConext) const;

protected:
    float _scale;
};

inline QDebug& AACubeShape::dumpToDebug(QDebug& debugConext) const {
    debugConext << "AACubeShape[ (" 
            << "type: " << getType()
            << "position: "
            << getTranslation().x << ", " << getTranslation().y << ", " << getTranslation().z
            << "scale: "
            << getScale()
            << "]";

    return debugConext;
}

#endif // hifi_AACubeShape_h
