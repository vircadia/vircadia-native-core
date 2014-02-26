//
//  ListShape.h
//
//  ListShape: A collection of shapes, each with a local transform.
//
//  Created by Andrew Meadows on 2014.02.20
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ListShape__
#define __hifi__ListShape__

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

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);

    const Shape* getSubShape(int index) const;

    void updateSubTransforms();

    int size() const { return _subShapeEntries.size(); }

    void addShape(Shape* shape, const glm::vec3& localPosition, const glm::quat& localRotation);

    void setShapes(QVector<ListShapeEntry>& shapes);

protected:
    void clear();
    void computeBoundingRadius();
    
    QVector<ListShapeEntry> _subShapeEntries;
    bool _subShapeTransformsAreDirty;

private:
    ListShape(const ListShape& otherList);  // don't implement this
};

#endif // __hifi__ListShape__
