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
#include "SpatialParentFinder.h"


class SpatiallyNestable;
using SpatiallyNestableWeakPointer = std::weak_ptr<SpatiallyNestable>;
using SpatiallyNestableWeakConstPointer = std::weak_ptr<const SpatiallyNestable>;
using SpatiallyNestablePointer = std::shared_ptr<SpatiallyNestable>;
using SpatiallyNestableConstPointer = std::shared_ptr<const SpatiallyNestable>;

class NestableTypes {
public:
    using NestableType = enum NestableType_t {
        Entity,
        Avatar
    };
};

class SpatiallyNestable : public std::enable_shared_from_this<SpatiallyNestable> {
public:
    // SpatiallyNestable() : _transform() { } // XXX get rid of this one?
    SpatiallyNestable(NestableTypes::NestableType nestableType, QUuid id) :
        _nestableType(nestableType),
        _id(id),
        _transform() { }
    virtual ~SpatiallyNestable() { }

    virtual const QUuid& getID() const { return _id; }
    virtual void setID(const QUuid& id) { _id = id; }

    virtual const QUuid& getParentID() const { return _parentID; }
    virtual void setParentID(const QUuid& parentID);

    virtual quint16 getParentJointIndex() const { return _parentJointIndex; }
    virtual void setParentJointIndex(quint16 parentJointIndex) { _parentJointIndex = parentJointIndex; }

    // world frame
    virtual const Transform& getTransform() const;
    virtual void setTransform(const Transform& transform);

    virtual Transform getParentTransform() const;

    virtual const glm::vec3& getPosition() const;
    virtual void setPosition(const glm::vec3& position);

    virtual const glm::quat& getOrientation() const;
    virtual void setOrientation(const glm::quat& orientation);

    virtual const glm::vec3& getScale() const;
    virtual void setScale(const glm::vec3& scale);

    // object's parent's frame
    virtual const Transform& getLocalTransform() const;
    virtual void setLocalTransform(const Transform& transform);

    virtual const glm::vec3& getLocalPosition() const;
    virtual void setLocalPosition(const glm::vec3& position);

    virtual const glm::quat& getLocalOrientation() const;
    virtual void setLocalOrientation(const glm::quat& orientation);

    virtual const glm::vec3& getLocalScale() const;
    virtual void setLocalScale(const glm::vec3& scale);

    QList<SpatiallyNestablePointer> getChildren() const;
    NestableTypes::NestableType getNestableType() const { return _nestableType; }

protected:
    bool _constructing = true;

    NestableTypes::NestableType _nestableType; // EntityItem or an AvatarData
    QUuid _id;
    QUuid _parentID; // what is this thing's transform relative to?
    quint16 _parentJointIndex; // which joint of the parent is this relative to?
    SpatiallyNestablePointer getParentPointer() const;
    mutable SpatiallyNestableWeakPointer _parent;

    virtual void beParentOfChild(SpatiallyNestablePointer newChild) const;
    virtual void forgetChild(SpatiallyNestablePointer newChild) const;
    mutable QHash<QUuid, SpatiallyNestableWeakPointer> _children;

private:
    Transform _transform; // this is to be combined with parent's world-transform to produce this' world-transform.

    // these are so we can return by reference
    mutable glm::vec3 _absolutePositionCache;
    mutable glm::quat _absoluteRotationCache;
    mutable Transform _worldTransformCache;
    mutable bool _parentKnowsMe = false;
};


#endif // hifi_SpatiallyNestable_h
