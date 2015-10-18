//
//  SpatiallyNestable.h
//  libraries/shared/src/
//
//  Created by Seth Alves on 2015-10-18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpatiallyNestable_h
#define hifi_SpatiallyNestable_h

#include <QUuid>

#include "Transform.h"


class SpatiallyNestable;
typedef std::weak_ptr<SpatiallyNestable> SpatiallyNestableWeakPointer;
typedef std::shared_ptr<SpatiallyNestable> SpatiallyNestablePointer;

class SpatiallyNestable {

public:
    SpatiallyNestable();
    virtual ~SpatiallyNestable() { }

    // world frame
    virtual const Transform& getTransform() const;
    virtual void setTransform(const Transform& transform);

    virtual const glm::vec3& getPosition() const;
    virtual void setPosition(const glm::vec3& position);

    virtual const glm::quat& getOrientation() const;
    virtual void setOrientation(const glm::quat& orientation);

    virtual const glm::vec3& getScale() const;
    virtual void setScale(const glm::vec3& scale);

    // model frame
    // ...


protected:
    QUuid _parentID; // what is this thing's transform relative to?
    int _parentJointIndex; // which joint of the parent is this relative to?

    SpatiallyNestableWeakPointer _parent;
    QVector<SpatiallyNestableWeakPointer> _children;

private:
    Transform _transform;
};


#endif // hifi_SpatiallyNestable_h
