//
//  ListShape.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/20/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  ListShape: A collection of shapes, each with a local transform.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ListShape_h
#define hifi_ListShape_h

#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#include "Shape.h"


class ListShapeEntry {
public:
    void updateTransform(const glm::vec3& position, const glm::quat& rotation);

    Shape* _shape;
    glm::vec3 _localPosition;
    glm::quat _localRotation;
};

class ListShape : public Shape {
public:

    ListShape() : Shape(LIST_SHAPE), _subShapeEntries(), _subShapeTransformsAreDirty(false) {}

    ListShape(const glm::vec3& position, const glm::quat& rotation) : 
        Shape(LIST_SHAPE, position, rotation), _subShapeEntries(), _subShapeTransformsAreDirty(false) {}

    ~ListShape();

    void setTranslation(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);

    const Shape* getSubShape(int index) const;

    void updateSubTransforms();

    int size() const { return _subShapeEntries.size(); }

    void addShape(Shape* shape, const glm::vec3& localPosition, const glm::quat& localRotation);

    void setShapes(QVector<ListShapeEntry>& shapes);

    // TODO: either implement this or remove ListShape altogether
    virtual bool findRayIntersection(RayIntersectionInfo& intersection) const { return false; }

protected:
    void clear();
    void computeBoundingRadius();
    
    QVector<ListShapeEntry> _subShapeEntries;
    bool _subShapeTransformsAreDirty;

private:
    ListShape(const ListShape& otherList);  // don't implement this
};

#endif // hifi_ListShape_h
