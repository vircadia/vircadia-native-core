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
#include "AACube.h"
#include "SpatialParentFinder.h"
#include "shared/ReadWriteLockable.h"


class SpatiallyNestable;
using SpatiallyNestableWeakPointer = std::weak_ptr<SpatiallyNestable>;
using SpatiallyNestableWeakConstPointer = std::weak_ptr<const SpatiallyNestable>;
using SpatiallyNestablePointer = std::shared_ptr<SpatiallyNestable>;
using SpatiallyNestableConstPointer = std::shared_ptr<const SpatiallyNestable>;

enum class NestableType {
    Entity,
    Avatar,
    Overlay
};

class SpatiallyNestable : public std::enable_shared_from_this<SpatiallyNestable> {
public:
    SpatiallyNestable(NestableType nestableType, QUuid id);
    virtual ~SpatiallyNestable();

    virtual const QUuid getID() const;
    virtual void setID(const QUuid& id);

    virtual const QUuid getParentID() const;
    virtual void setParentID(const QUuid& parentID);

    virtual quint16 getParentJointIndex() const { return _parentJointIndex; }
    virtual void setParentJointIndex(quint16 parentJointIndex);

    static glm::vec3 worldToLocal(const glm::vec3& position, const QUuid& parentID, int parentJointIndex, bool& success);
    static glm::quat worldToLocal(const glm::quat& orientation, const QUuid& parentID, int parentJointIndex, bool& success);
    static glm::vec3 worldToLocalVelocity(const glm::vec3& velocity, const QUuid& parentID,
                                          int parentJointIndex, bool& success);
    static glm::vec3 worldToLocalAngularVelocity(const glm::vec3& angularVelocity, const QUuid& parentID,
                                                 int parentJointIndex, bool& success);

    static glm::vec3 localToWorld(const glm::vec3& position, const QUuid& parentID, int parentJointIndex, bool& success);
    static glm::quat localToWorld(const glm::quat& orientation, const QUuid& parentID, int parentJointIndex, bool& success);
    static glm::vec3 localToWorldVelocity(const glm::vec3& velocity,
                                          const QUuid& parentID, int parentJointIndex, bool& success);
    static glm::vec3 localToWorldAngularVelocity(const glm::vec3& angularVelocity,
                                                 const QUuid& parentID, int parentJointIndex, bool& success);

    // world frame
    virtual const Transform getTransform(bool& success, int depth = 0) const;
    virtual const Transform getTransform() const;
    virtual void setTransform(const Transform& transform, bool& success);
    virtual bool setTransform(const Transform& transform);

    virtual Transform getParentTransform(bool& success, int depth = 0) const;

    virtual glm::vec3 getPosition(bool& success) const;
    virtual glm::vec3 getPosition() const;
    virtual void setPosition(const glm::vec3& position, bool& success, bool tellPhysics = true);
    virtual void setPosition(const glm::vec3& position);

    virtual glm::quat getOrientation(bool& success) const;
    virtual glm::quat getOrientation() const;
    virtual glm::quat getOrientation(int jointIndex, bool& success) const;
    virtual void setOrientation(const glm::quat& orientation, bool& success, bool tellPhysics = true);
    virtual void setOrientation(const glm::quat& orientation);

    // these are here because some older code uses rotation rather than orientation
    virtual const glm::quat getRotation() const { return getOrientation(); }
    virtual void setRotation(glm::quat orientation) { setOrientation(orientation); }

    virtual glm::vec3 getVelocity(bool& success) const;
    virtual glm::vec3 getVelocity() const;
    virtual void setVelocity(const glm::vec3& velocity, bool& success);
    virtual void setVelocity(const glm::vec3& velocity);
    virtual glm::vec3 getParentVelocity(bool& success) const;

    virtual glm::vec3 getAngularVelocity(bool& success) const;
    virtual glm::vec3 getAngularVelocity() const;
    virtual void setAngularVelocity(const glm::vec3& angularVelocity, bool& success);
    virtual void setAngularVelocity(const glm::vec3& angularVelocity);
    virtual glm::vec3 getParentAngularVelocity(bool& success) const;

    virtual AACube getMaximumAACube(bool& success) const;
    virtual void checkAndAdjustQueryAACube();
    virtual bool computePuffedQueryAACube();

    virtual void setQueryAACube(const AACube& queryAACube);
    virtual bool queryAABoxNeedsUpdate() const;
    virtual AACube getQueryAACube(bool& success) const;
    virtual AACube getQueryAACube() const;

    virtual glm::vec3 getScale() const;
    virtual void setScale(const glm::vec3& scale);
    virtual void setScale(float value);

    // get world-frame values for a specific joint
    virtual const Transform getTransform(int jointIndex, bool& success, int depth = 0) const;
    virtual glm::vec3 getPosition(int jointIndex, bool& success) const;
    virtual glm::vec3 getScale(int jointIndex) const;

    // object's parent's frame
    virtual const Transform getLocalTransform() const;
    virtual void setLocalTransform(const Transform& transform);

    virtual glm::vec3 getLocalPosition() const;
    virtual void setLocalPosition(const glm::vec3& position, bool tellPhysics = true);

    virtual glm::quat getLocalOrientation() const;
    virtual void setLocalOrientation(const glm::quat& orientation);

    virtual glm::vec3 getLocalVelocity() const;
    virtual void setLocalVelocity(const glm::vec3& velocity);

    virtual glm::vec3 getLocalAngularVelocity() const;
    virtual void setLocalAngularVelocity(const glm::vec3& angularVelocity);

    virtual glm::vec3 getLocalScale() const;
    virtual void setLocalScale(const glm::vec3& scale);

    QList<SpatiallyNestablePointer> getChildren() const;
    bool hasChildren() const;

    NestableType getNestableType() const { return _nestableType; }

    // this object's frame
    virtual const Transform getAbsoluteJointTransformInObjectFrame(int jointIndex) const;
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const { return glm::quat(); }
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const { return glm::vec3(); }
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) { return false; }
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) {return false; }

    SpatiallyNestablePointer getThisPointer() const;

    void markAncestorMissing(bool value) { _missingAncestor = value; }
    bool getAncestorMissing() { return _missingAncestor; }

    void forEachChild(std::function<void(SpatiallyNestablePointer)> actor);
    void forEachDescendant(std::function<void(SpatiallyNestablePointer)> actor);

    void die() { _isDead = true; }
    bool isDead() const { return _isDead; }

    bool isParentIDValid() const { bool success = false; getParentPointer(success); return success; }
    virtual SpatialParentTree* getParentTree() const { return nullptr; }

    bool hasAncestorOfType(NestableType nestableType);
    SpatiallyNestablePointer getParentPointer(bool& success) const;
    static SpatiallyNestablePointer findByID(QUuid id, bool& success);

    void getLocalTransformAndVelocities(Transform& localTransform,
                                        glm::vec3& localVelocity,
                                        glm::vec3& localAngularVelocity) const;

    void setLocalTransformAndVelocities(
            const Transform& localTransform,
            const glm::vec3& localVelocity,
            const glm::vec3& localAngularVelocity);

protected:
    const NestableType _nestableType; // EntityItem or an AvatarData
    QUuid _id;
    QUuid _parentID; // what is this thing's transform relative to?
    quint16 _parentJointIndex { 0 }; // which joint of the parent is this relative to?

    mutable SpatiallyNestableWeakPointer _parent;

    virtual void beParentOfChild(SpatiallyNestablePointer newChild) const;
    virtual void forgetChild(SpatiallyNestablePointer newChild) const;

    mutable ReadWriteLockable _childrenLock;
    mutable QHash<QUuid, SpatiallyNestableWeakPointer> _children;

    virtual void locationChanged(bool tellPhysics = true); // called when a this object's location has changed
    virtual void dimensionsChanged() { } // called when a this object's dimensions have changed
    virtual void parentDeleted() { } // called on children of a deleted parent

    // _queryAACube is used to decide where something lives in the octree
    mutable AACube _queryAACube;
    mutable bool _queryAACubeSet { false };

    bool _missingAncestor { false };

private:
    mutable ReadWriteLockable _transformLock;
    mutable ReadWriteLockable _idLock;
    mutable ReadWriteLockable _velocityLock;
    mutable ReadWriteLockable _angularVelocityLock;
    Transform _transform; // this is to be combined with parent's world-transform to produce this' world-transform.
    glm::vec3 _velocity;
    glm::vec3 _angularVelocity;
    mutable bool _parentKnowsMe { false };
    bool _isDead { false };
};


#endif // hifi_SpatiallyNestable_h
