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
#include "Grab.h"

class SpatiallyNestable;
using SpatiallyNestableWeakPointer = std::weak_ptr<SpatiallyNestable>;
using SpatiallyNestableWeakConstPointer = std::weak_ptr<const SpatiallyNestable>;
using SpatiallyNestablePointer = std::shared_ptr<SpatiallyNestable>;
using SpatiallyNestableConstPointer = std::shared_ptr<const SpatiallyNestable>;

static const uint16_t INVALID_JOINT_INDEX = -1;

enum class NestableType {
    Entity,
    Avatar
};

class SpatiallyNestable : public std::enable_shared_from_this<SpatiallyNestable> {
public:
    SpatiallyNestable(NestableType nestableType, QUuid id);
    virtual ~SpatiallyNestable();

    virtual const QUuid getID() const;
    virtual void setID(const QUuid& id);

    virtual QString getName() const { return "SpatiallyNestable"; }

    virtual const QUuid getParentID() const;
    virtual void setParentID(const QUuid& parentID);

    virtual bool isMyAvatar() const { return false; }

    virtual quint16 getParentJointIndex() const { return _parentJointIndex; }
    virtual void setParentJointIndex(quint16 parentJointIndex);

    static glm::vec3 worldToLocal(const glm::vec3& position, const QUuid& parentID, int parentJointIndex,
                                  bool scalesWithParent, bool& success);
    static glm::quat worldToLocal(const glm::quat& orientation, const QUuid& parentID, int parentJointIndex,
                                  bool scalesWithParent, bool& success);
    static glm::vec3 worldToLocalVelocity(const glm::vec3& velocity, const QUuid& parentID,
                                          int parentJointIndex, bool scalesWithParent, bool& success);
    static glm::vec3 worldToLocalAngularVelocity(const glm::vec3& angularVelocity, const QUuid& parentID,
                                                 int parentJointIndex, bool scalesWithParent, bool& success);
    static glm::vec3 worldToLocalDimensions(const glm::vec3& dimensions, const QUuid& parentID,
                                            int parentJointIndex, bool scalesWithParent, bool& success);

    static glm::vec3 localToWorld(const glm::vec3& position, const QUuid& parentID, int parentJointIndex,
                                  bool scalesWithParent, bool& success);
    static glm::quat localToWorld(const glm::quat& orientation, const QUuid& parentID, int parentJointIndex,
                                  bool scalesWithParent, bool& success);
    static glm::vec3 localToWorldVelocity(const glm::vec3& velocity,
                                          const QUuid& parentID, int parentJointIndex, bool scalesWithParent, bool& success);
    static glm::vec3 localToWorldAngularVelocity(const glm::vec3& angularVelocity,
                                                 const QUuid& parentID, int parentJointIndex,
                                                 bool scalesWithParent, bool& success);
    static glm::vec3 localToWorldDimensions(const glm::vec3& dimensions, const QUuid& parentID,
                                            int parentJointIndex, bool scalesWithParent, bool& success);

    static QString nestableTypeToString(NestableType nestableType);


    virtual bool isParentPathComplete(int depth = 0) const;


    // world frame
    virtual const Transform getTransform(bool& success, int depth = 0) const;
    virtual const Transform getTransformWithOnlyLocalRotation(bool& success, int depth = 0) const;
    virtual const Transform getTransform() const;
    virtual void setTransform(const Transform& transform, bool& success);
    virtual bool setTransform(const Transform& transform);

    virtual Transform getParentTransform(bool& success, int depth = 0) const;

    void setWorldTransform(const glm::vec3& position, const glm::quat& orientation);
    virtual glm::vec3 getWorldPosition(bool& success) const;
    virtual glm::vec3 getWorldPosition() const;
    virtual void setWorldPosition(const glm::vec3& position, bool& success, bool tellPhysics = true);
    virtual void setWorldPosition(const glm::vec3& position);

    virtual glm::quat getWorldOrientation(bool& success) const;
    virtual glm::quat getWorldOrientation() const;
    virtual glm::quat getWorldOrientation(int jointIndex, bool& success) const;
    virtual void setWorldOrientation(const glm::quat& orientation, bool& success, bool tellPhysics = true);
    virtual void setWorldOrientation(const glm::quat& orientation);

    virtual glm::vec3 getWorldVelocity(bool& success) const;
    virtual glm::vec3 getWorldVelocity() const;
    virtual void setWorldVelocity(const glm::vec3& velocity, bool& success);
    virtual void setWorldVelocity(const glm::vec3& velocity);
    virtual glm::vec3 getParentVelocity(bool& success) const;

    virtual glm::vec3 getWorldAngularVelocity(bool& success) const;
    virtual glm::vec3 getWorldAngularVelocity() const;
    virtual void setWorldAngularVelocity(const glm::vec3& angularVelocity, bool& success);
    virtual void setWorldAngularVelocity(const glm::vec3& angularVelocity);
    virtual glm::vec3 getParentAngularVelocity(bool& success) const;

    virtual AACube getMaximumAACube(bool& success) const;
    virtual AACube calculateInitialQueryAACube(bool& success);

    virtual void setQueryAACube(const AACube& queryAACube);
    virtual bool queryAACubeNeedsUpdate() const;
    virtual bool queryAACubeNeedsUpdateWithDescendantAACube(const AACube& descendantAACube) const;
    virtual bool shouldPuffQueryAACube() const { return false; }
    bool updateQueryAACube(bool updateParent = true);
    bool updateQueryAACubeWithDescendantAACube(const AACube& descendentAACube, bool updateParent = true);
    void forceQueryAACubeUpdate() { _queryAACubeSet = false; }
    virtual AACube getQueryAACube(bool& success) const;
    virtual AACube getQueryAACube() const;

    virtual glm::vec3 getSNScale() const;
    virtual glm::vec3 getSNScale(bool& success) const;
    virtual void setSNScale(const glm::vec3& scale);
    virtual void setSNScale(const glm::vec3& scale, bool& success);

    // get world-frame values for a specific joint
    virtual const Transform getJointTransform(int jointIndex, bool& success, int depth = 0) const;
    virtual glm::vec3 getJointWorldPosition(int jointIndex, bool& success) const;
    virtual glm::vec3 getJointSNScale(int jointIndex, bool& success) const;

    // object's parent's frame
    virtual Transform getLocalTransform() const;
    virtual void setLocalTransform(const Transform& transform);

    virtual glm::vec3 getLocalPosition() const;
    virtual void setLocalPosition(const glm::vec3& position, bool tellPhysics = true);

    virtual glm::quat getLocalOrientation() const;
    virtual void setLocalOrientation(const glm::quat& orientation);

    virtual glm::vec3 getLocalVelocity() const;
    virtual void setLocalVelocity(const glm::vec3& velocity);

    virtual glm::vec3 getLocalAngularVelocity() const;
    virtual void setLocalAngularVelocity(const glm::vec3& angularVelocity);

    virtual glm::vec3 getLocalSNScale() const;
    virtual void setLocalSNScale(const glm::vec3& scale);

    virtual bool getScalesWithParent() const { return false; }
    virtual glm::vec3 scaleForChildren() const { return glm::vec3(1.0f); }

    QList<SpatiallyNestablePointer> getChildren() const;
    bool hasChildren() const;

    NestableType getNestableType() const { return _nestableType; }

    // this object's frame
    virtual const Transform getAbsoluteJointTransformInObjectFrame(int jointIndex) const;
    virtual glm::vec3 getAbsoluteJointScaleInObjectFrame(int index) const { return glm::vec3(1.0f); }
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const { return glm::quat(); }
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const { return glm::vec3(); }
    virtual int getJointParent(int index) const { return -1; }

    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) { return false; }
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) {return false; }

    virtual glm::quat getLocalJointRotation(int index) const {return glm::quat(); }
    virtual glm::vec3 getLocalJointTranslation(int index) const {return glm::vec3(); }
    virtual bool setLocalJointRotation(int index, const glm::quat& rotation) { return false; }
    virtual bool setLocalJointTranslation(int index, const glm::vec3& translation) { return false; }

    SpatiallyNestablePointer getThisPointer() const;

    using ChildLambda = std::function<void(const SpatiallyNestablePointer&)>;
    using ChildLambdaTest = std::function<bool(const SpatiallyNestablePointer&)>;

    void forEachChild(const ChildLambda& actor) const;
    void forEachDescendant(const ChildLambda& actor) const;
    void forEachChildTest(const ChildLambdaTest&  actor) const;
    void forEachDescendantTest(const ChildLambdaTest& actor) const;

    void die() { _isDead = true; }
    bool isDead() const { return _isDead; }

    bool isParentIDValid() const { bool success = false; getParentPointer(success); return success; }
    virtual SpatialParentTree* getParentTree() const { return nullptr; }

    bool hasAncestorOfType(NestableType nestableType, int depth = 0) const;
    const QUuid findAncestorOfType(NestableType nestableType, int depth = 0) const;
    SpatiallyNestablePointer getParentPointer(bool& success) const;
    static SpatiallyNestablePointer findByID(QUuid id, bool& success);

    void getLocalTransformAndVelocities(Transform& localTransform,
                                        glm::vec3& localVelocity,
                                        glm::vec3& localAngularVelocity) const;

    void setLocalTransformAndVelocities(
            const Transform& localTransform,
            const glm::vec3& localVelocity,
            const glm::vec3& localAngularVelocity);

    bool scaleChangedSince(quint64 time) const { return _scaleChanged > time; }
    bool tranlationChangedSince(quint64 time) const { return _translationChanged > time; }
    bool rotationChangedSince(quint64 time) const { return _rotationChanged > time; }

    void dump(const QString& prefix = "") const;

    virtual void locationChanged(bool tellPhysics = true, bool tellChildren = true); // called when a this object's location has changed
    virtual void dimensionsChanged() { _queryAACubeSet = false; } // called when a this object's dimensions have changed
    virtual void parentDeleted() { } // called on children of a deleted parent

    virtual void addGrab(GrabPointer grab);
    virtual void removeGrab(GrabPointer grab);
    virtual void disableGrab(GrabPointer grab) {};
    bool hasGrabs();
    virtual QUuid getEditSenderID();

    void bumpAncestorChainRenderableVersion(int depth = 0) const;

protected:
    QUuid _id;
    mutable SpatiallyNestableWeakPointer _parent;

    virtual void beParentOfChild(SpatiallyNestablePointer newChild) const;
    virtual void forgetChild(SpatiallyNestablePointer newChild) const;
    virtual void recalculateChildCauterization() const { }

    mutable ReadWriteLockable _childrenLock;
    mutable QHash<QUuid, SpatiallyNestableWeakPointer> _children;

    // _queryAACube is used to decide where something lives in the octree
    mutable AACube _queryAACube;
    mutable bool _queryAACubeSet { false };

    quint64 _scaleChanged { 0 };
    quint64 _translationChanged { 0 };
    quint64 _rotationChanged { 0 };

    mutable ReadWriteLockable _grabsLock;
    QSet<GrabPointer> _grabs; // upon this thing

    mutable std::atomic<uint32_t> _ancestorChainRenderableVersion { 0 };

private:
    SpatiallyNestable() = delete;
    const NestableType _nestableType; // EntityItem or an AvatarData
    QUuid _parentID; // what is this thing's transform relative to?
    quint16 _parentJointIndex { INVALID_JOINT_INDEX }; // which joint of the parent is this relative to?

    mutable ReadWriteLockable _transformLock;
    mutable ReadWriteLockable _idLock;
    mutable ReadWriteLockable _velocityLock;
    mutable ReadWriteLockable _angularVelocityLock;
    Transform _transform; // this is to be combined with parent's world-transform to produce this' world-transform.
    glm::vec3 _velocity;
    glm::vec3 _angularVelocity;
    mutable bool _parentKnowsMe { false };
    bool _isDead { false };
    bool _queryAACubeIsPuffed { false };

    void breakParentingLoop() const;
};


#endif // hifi_SpatiallyNestable_h
