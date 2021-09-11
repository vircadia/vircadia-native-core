//
//  SpatiallyNestable.cpp
//  libraries/shared/src/
//
//  Created by Seth Alves on 2015-10-18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SpatiallyNestable.h"

#include <QtCore/QSharedPointer>

#include <queue>

#include "DependencyManager.h"
#include "SharedUtil.h"
#include "StreamUtils.h"
#include "SharedLogging.h"

const float defaultAACubeSize = 1.0f;
const int MAX_PARENTING_CHAIN_SIZE = 30;

SpatiallyNestable::SpatiallyNestable(NestableType nestableType, QUuid id) :
    _id(id),
    _nestableType(nestableType),
    _transform() {
    // set flags in _transform
    _transform.setTranslation(glm::vec3(0.0f));
    _transform.setRotation(glm::quat());
    _transform.setScale(1.0f);
    _scaleChanged = usecTimestampNow();
    _translationChanged = usecTimestampNow();
    _rotationChanged = usecTimestampNow();
}

SpatiallyNestable::~SpatiallyNestable() {
    forEachChild([&](SpatiallyNestablePointer object) {
        object->parentDeleted();
    });
}

const QUuid SpatiallyNestable::getID() const {
    QUuid result;
    _idLock.withReadLock([&] {
        result = _id;
    });
    return result;
}

void SpatiallyNestable::setID(const QUuid& id) {
    // adjust the parentID of any children
    forEachChild([&](SpatiallyNestablePointer object) {
        object->setParentID(id);
    });
    _idLock.withWriteLock([&] {
        _id = id;
    });
}

const QUuid SpatiallyNestable::getParentID() const {
    QUuid result;
    _idLock.withReadLock([&] {
        result = _parentID;
    });
    return result;
}

void SpatiallyNestable::setParentID(const QUuid& parentID) {
    bumpAncestorChainRenderableVersion();
    bool success = false;
    auto parent = getParentPointer(success);
    bool parentChanged = false;
    _idLock.withWriteLock([&] {
        if (_parentID != parentID) {
            parentChanged = true;
            _parentID = parentID;
            _parentKnowsMe = false;
        }
    });

    if (parentChanged && success && parent) {
        parent->recalculateChildCauterization();
    }

    if (!_parentKnowsMe) {
        success = false;
        parent = getParentPointer(success);
        if (success && parent) {
            bumpAncestorChainRenderableVersion();
            parent->updateQueryAACube();
            parent->recalculateChildCauterization();
        }
    }
}

Transform SpatiallyNestable::getParentTransform(bool& success, int depth) const {
    Transform result;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success) {
        return result;
    }
    if (parent) {
        result = parent->getJointTransform(_parentJointIndex, success, depth + 1);
        if (getScalesWithParent()) {
            result.setScale(parent->scaleForChildren());
        }
    }
    return result;
}

SpatiallyNestablePointer SpatiallyNestable::getParentPointer(bool& success) const {
    SpatiallyNestablePointer parent = _parent.lock();
    QUuid parentID = getParentID(); // used for its locking

    if (!parent && parentID.isNull()) {
        // no parent
        success = true;
        return nullptr;
    }

    if (parent && (parent->getID() == parentID ||
                   (parentID == AVATAR_SELF_ID && parent->isMyAvatar()))) {
        // parent pointer is up-to-date
        if (!_parentKnowsMe) {
            SpatialParentTree* parentTree = parent->getParentTree();
            if (!parentTree || parentTree == getParentTree()) {
                parent->beParentOfChild(getThisPointer());
                _parentKnowsMe = true;
            }
        }
        success = true;
        return parent;
    }

    if (parent) {
        // we have a parent pointer but our _parentID doesn't indicate this parent.
        parent->forgetChild(getThisPointer());
        _parentKnowsMe = false;
        _parent.reset();
    }

    // we have a _parentID but no parent pointer, or our parent pointer was to the wrong thing
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return nullptr;
    }
    _parent = parentFinder->find(parentID, success, getParentTree());
    if (!success) {
        return nullptr;
    }

    parent = _parent.lock();
    if (parent) {

        // it's possible for an entity with a parent of AVATAR_SELF_ID can be imported into a side-tree
        // such as the clipboard's.  if this is the case, we don't want the parent to consider this a
        // child.
        SpatialParentTree* parentTree = parent->getParentTree();
        if (!parentTree || parentTree == getParentTree()) {
            parent->beParentOfChild(getThisPointer());
            _parentKnowsMe = true;
        }
    }

    success = (parent || parentID.isNull());
    return parent;
}

void SpatiallyNestable::beParentOfChild(SpatiallyNestablePointer newChild) const {
    _childrenLock.withWriteLock([&] {
        _children[newChild->getID()] = newChild;
    });
    // Our QueryAACube will automatically be updated to include our new child
}

void SpatiallyNestable::forgetChild(SpatiallyNestablePointer newChild) const {
    _childrenLock.withWriteLock([&] {
        _children.remove(newChild->getID());
    });
    _queryAACubeSet = false; // We need to reset our queryAACube when we lose a child
}

void SpatiallyNestable::setParentJointIndex(quint16 parentJointIndex) {
    _parentJointIndex = parentJointIndex;
    bool success = false;
    auto parent = getParentPointer(success);
    if (success && parent) {
        parent->recalculateChildCauterization();
    }
}

glm::vec3 SpatiallyNestable::worldToLocal(const glm::vec3& position,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool scalesWithParent, bool& success) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::vec3(0.0f);
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::vec3(0.0f);
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::vec3(0.0f);
    }

    if (parent) {
        parentTransform = parent->getJointTransform(parentJointIndex, success);
        if (!success) {
            return glm::vec3(0.0f);
        }
        if (scalesWithParent) {
            parentTransform.setScale(parent->scaleForChildren());
        }
    }
    success = true;

    Transform invParentTransform;
    parentTransform.evalInverse(invParentTransform);
    return invParentTransform.transform(position);
}

glm::quat SpatiallyNestable::worldToLocal(const glm::quat& orientation,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool scalesWithParent, bool& success) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::quat();
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::quat();
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::quat();
    }

    if (parent) {
        parentTransform = parent->getJointTransform(parentJointIndex, success);
        if (!success) {
            return glm::quat();
        }
        if (scalesWithParent) {
            parentTransform.setScale(parent->scaleForChildren());
        }
    }
    success = true;

    glm::quat invParentOrientation = glm::inverse(parentTransform.getRotation());
    return invParentOrientation * orientation;
}

glm::vec3 SpatiallyNestable::worldToLocalVelocity(const glm::vec3& velocity, const QUuid& parentID,
                                                  int parentJointIndex, bool scalesWithParent, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return velocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return velocity;
    }
    if (scalesWithParent) {
        parentTransform.setScale(parent->scaleForChildren());
    }
    glm::vec3 parentVelocity = parent->getWorldVelocity(success);
    if (!success) {
        return velocity;
    }

    return glm::inverse(parentTransform.getRotation()) * (velocity - parentVelocity);
}

glm::vec3 SpatiallyNestable::worldToLocalAngularVelocity(const glm::vec3& angularVelocity, const QUuid& parentID,
                                                         int parentJointIndex, bool scalesWithParent, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return angularVelocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return angularVelocity;
    }
    if (scalesWithParent) {
        parentTransform.setScale(parent->scaleForChildren());
    }

    return glm::inverse(parentTransform.getRotation()) * angularVelocity;
}


glm::vec3 SpatiallyNestable::worldToLocalDimensions(const glm::vec3& dimensions,
                                                    const QUuid& parentID, int parentJointIndex,
                                                    bool scalesWithParent, bool& success) {
    if (!scalesWithParent) {
        success = true;
        return dimensions;
    }

    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return dimensions;
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return dimensions;
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return dimensions;
    }

    success = true;
    if (parent) {
        return dimensions / parent->scaleForChildren();
    }
    return dimensions;
}

glm::vec3 SpatiallyNestable::localToWorld(const glm::vec3& position,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool scalesWithParent,
                                          bool& success) {
    Transform result;
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::vec3(0.0f);
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::vec3(0.0f);
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::vec3(0.0f);
    }

    if (parent) {
        parentTransform = parent->getJointTransform(parentJointIndex, success);
        if (!success) {
            return glm::vec3(0.0f);
        }
        if (scalesWithParent) {
            parentTransform.setScale(parent->scaleForChildren());
        }
    }
    success = true;

    Transform positionTransform;
    positionTransform.setTranslation(position);
    Transform::mult(result, parentTransform, positionTransform);
    return result.getTranslation();
}

glm::quat SpatiallyNestable::localToWorld(const glm::quat& orientation,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool scalesWithParent,
                                          bool& success) {
    Transform result;
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::quat();
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::quat();
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::quat();
    }

    if (parent) {
        parentTransform = parent->getJointTransform(parentJointIndex, success);
        if (!success) {
            return glm::quat();
        }
        if (scalesWithParent) {
            parentTransform.setScale(parent->scaleForChildren());
        }
    }
    success = true;

    Transform orientationTransform;
    orientationTransform.setRotation(orientation);
    Transform::mult(result, parentTransform, orientationTransform);
    return result.getRotation();
}

glm::vec3 SpatiallyNestable::localToWorldVelocity(const glm::vec3& velocity, const QUuid& parentID,
                                                  int parentJointIndex, bool scalesWithParent, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return velocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return velocity;
    }
    if (scalesWithParent) {
        parentTransform.setScale(parent->scaleForChildren());
    }
    glm::vec3 parentVelocity = parent->getWorldVelocity(success);
    if (!success) {
        return velocity;
    }

    return parentVelocity + parentTransform.getRotation() * velocity;
}

glm::vec3 SpatiallyNestable::localToWorldAngularVelocity(const glm::vec3& angularVelocity, const QUuid& parentID,
                                                         int parentJointIndex, bool scalesWithParent, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return angularVelocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return angularVelocity;
    }
    if (scalesWithParent) {
        parentTransform.setScale(parent->scaleForChildren());
    }
    return parentTransform.getRotation() * angularVelocity;
}


glm::vec3 SpatiallyNestable::localToWorldDimensions(const glm::vec3& dimensions,
                                                    const QUuid& parentID, int parentJointIndex, bool scalesWithParent,
                                                    bool& success) {
    if (!scalesWithParent) {
        success = true;
        return dimensions;
    }

    Transform result;
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return dimensions;
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return dimensions;
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return dimensions;
    }

    success = true;
    if (parent) {
        return dimensions * parent->scaleForChildren();
    }
    return dimensions;
}

void SpatiallyNestable::setWorldTransform(const glm::vec3& position, const glm::quat& orientation) {
    // guard against introducing NaN into the transform
    if (isNaN(orientation) || isNaN(position)) {
        return;
    }

    bool success = true;
    Transform parentTransform = getParentTransform(success);
    if (success) {
        bool changed = false;
        _transformLock.withWriteLock([&] {
            Transform myWorldTransform;
            Transform::mult(myWorldTransform, parentTransform, _transform);
            if (myWorldTransform.getRotation() != orientation) {
                changed = true;
                myWorldTransform.setRotation(orientation);
            }
            if (myWorldTransform.getTranslation() != position) {
                changed = true;
                myWorldTransform.setTranslation(position);
            }
            if (changed) {
                Transform::inverseMult(_transform, parentTransform, myWorldTransform);
                _translationChanged = usecTimestampNow();
            }
        });
        if (changed) {
            locationChanged(false);
        }
    }
}

glm::vec3 SpatiallyNestable::getWorldPosition(bool& success) const {
    return getTransform(success).getTranslation();
}

glm::vec3 SpatiallyNestable::getWorldPosition() const {
    bool success;
    auto result = getWorldPosition(success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- getWorldPosition failed" << getID();
    }
    #endif
    return result;
}

glm::vec3 SpatiallyNestable::getJointWorldPosition(int jointIndex, bool& success) const {
    return getJointTransform(jointIndex, success).getTranslation();
}

void SpatiallyNestable::setWorldPosition(const glm::vec3& position, bool& success, bool tellPhysics) {
    // guard against introducing NaN into the transform
    if (isNaN(position)) {
        success = false;
        return;
    }

    bool changed = false;
    Transform parentTransform = getParentTransform(success);
    Transform myWorldTransform;
    _transformLock.withWriteLock([&] {
        Transform::mult(myWorldTransform, parentTransform, _transform);
        if (myWorldTransform.getTranslation() != position) {
            changed = true;
            myWorldTransform.setTranslation(position);
            Transform::inverseMult(_transform, parentTransform, myWorldTransform);
            _translationChanged = usecTimestampNow();
        }
    });
    if (success && changed) {
        locationChanged(tellPhysics);
    }
}

void SpatiallyNestable::setWorldPosition(const glm::vec3& position) {
    bool success;
    setWorldPosition(position, success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- setWorldPosition failed" << getID();
    }
    #endif
}

glm::quat SpatiallyNestable::getWorldOrientation(bool& success) const {
    return getTransform(success).getRotation();
}

glm::quat SpatiallyNestable::getWorldOrientation() const {
    bool success;
    auto result = getWorldOrientation(success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- getOrientation failed" << getID();
    }
    #endif
    return result;
}

glm::quat SpatiallyNestable::getWorldOrientation(int jointIndex, bool& success) const {
    return getJointTransform(jointIndex, success).getRotation();
}

void SpatiallyNestable::setWorldOrientation(const glm::quat& orientation, bool& success, bool tellPhysics) {
    // guard against introducing NaN into the transform
    if (isNaN(orientation)) {
        success = false;
        return;
    }

    bool changed = false;
    Transform parentTransform = getParentTransform(success);
    Transform myWorldTransform;
    _transformLock.withWriteLock([&] {
        Transform::mult(myWorldTransform, parentTransform, _transform);
        if (myWorldTransform.getRotation() != orientation) {
            changed = true;
            myWorldTransform.setRotation(orientation);
            Transform::inverseMult(_transform, parentTransform, myWorldTransform);
            _rotationChanged = usecTimestampNow();
        }
    });
    if (success && changed) {
        locationChanged(tellPhysics);
    }
}

void SpatiallyNestable::setWorldOrientation(const glm::quat& orientation) {
    bool success;
    setWorldOrientation(orientation, success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- setOrientation failed" << getID();
    }
    #endif
}

glm::vec3 SpatiallyNestable::getWorldVelocity(bool& success) const {
    glm::vec3 result;
    Transform parentTransform = getParentTransform(success);
    if (!success) {
        return result;
    }
    glm::vec3 parentVelocity = getParentVelocity(success);
    if (!success) {
        return result;
    }
    _velocityLock.withReadLock([&] {
        // TODO: take parent angularVelocity into account.
        result = parentVelocity + parentTransform.getRotation() * _velocity;
    });
    return result;
}

glm::vec3 SpatiallyNestable::getWorldVelocity() const {
    bool success;
    glm::vec3 result = getWorldVelocity(success);
    if (!success) {
        qCDebug(shared) << "Warning -- getWorldVelocity failed" << getID();
    }
    return result;
}

void SpatiallyNestable::setWorldVelocity(const glm::vec3& velocity, bool& success) {
    glm::vec3 parentVelocity = getParentVelocity(success);
    Transform parentTransform = getParentTransform(success);
    _velocityLock.withWriteLock([&] {
        // HACK: until we are treating _velocity the same way we treat _position (meaning,
        // _velocity is a vs parent value and any request for a world-frame velocity must
        // be computed), do this to avoid equipped (parenting-grabbed) things from drifting.
        // turning a zero velocity into a non-zero _velocity (because the avatar is moving)
        // causes EntityItem::stepKinematicMotion to have an effect on the equipped entity,
        // which causes it to drift from the hand.
        if (hasAncestorOfType(NestableType::Avatar)) {
            _velocity = velocity;
        } else {
            // TODO: take parent angularVelocity into account.
            _velocity = glm::inverse(parentTransform.getRotation()) * (velocity - parentVelocity);
        }
    });
}

void SpatiallyNestable::setWorldVelocity(const glm::vec3& velocity) {
    bool success;
    setWorldVelocity(velocity, success);
    if (!success) {
        qCDebug(shared) << "Warning -- setVelocity failed" << getID();
    }
}

glm::vec3 SpatiallyNestable::getParentVelocity(bool& success) const {
    glm::vec3 result;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success) {
        return result;
    }
    if (parent) {
        result = parent->getWorldVelocity(success);
    }
    return result;
}

glm::vec3 SpatiallyNestable::getWorldAngularVelocity(bool& success) const {
    glm::vec3 result;
    Transform parentTransform = getParentTransform(success);
    if (!success) {
        return result;
    }
    glm::vec3 parentAngularVelocity = getParentAngularVelocity(success);
    if (!success) {
        return result;
    }
    _angularVelocityLock.withReadLock([&] {
        result = parentAngularVelocity + parentTransform.getRotation() * _angularVelocity;
    });
    return result;
}

glm::vec3 SpatiallyNestable::getWorldAngularVelocity() const {
    bool success;
    glm::vec3 result = getWorldAngularVelocity(success);
    if (!success) {
        qCDebug(shared) << "Warning -- getAngularVelocity failed" << getID();
    }
    return result;
}

void SpatiallyNestable::setWorldAngularVelocity(const glm::vec3& angularVelocity, bool& success) {
    glm::vec3 parentAngularVelocity = getParentAngularVelocity(success);
    Transform parentTransform = getParentTransform(success);
    _angularVelocityLock.withWriteLock([&] {
        _angularVelocity = glm::inverse(parentTransform.getRotation()) * (angularVelocity - parentAngularVelocity);
    });
}

void SpatiallyNestable::setWorldAngularVelocity(const glm::vec3& angularVelocity) {
    bool success;
    setWorldAngularVelocity(angularVelocity, success);
    if (!success) {
        qCDebug(shared) << "Warning -- setAngularVelocity failed" << getID();
    }
}

glm::vec3 SpatiallyNestable::getParentAngularVelocity(bool& success) const {
    glm::vec3 result;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success) {
        return result;
    }
    if (parent) {
        result = parent->getWorldAngularVelocity(success);
    }
    return result;
}

const Transform SpatiallyNestable::getTransform(bool& success, int depth) const {
    Transform result;
    // return a world-space transform for this object's location
    Transform parentTransform = getParentTransform(success, depth);
    _transformLock.withReadLock([&] {
        Transform::mult(result, parentTransform, _transform);
    });
    return result;
}

const Transform SpatiallyNestable::getTransformWithOnlyLocalRotation(bool& success, int depth) const {
    Transform result;
    // return a world-space transform for this object's location
    Transform parentTransform = getParentTransform(success, depth);
    _transformLock.withReadLock([&] {
        Transform::mult(result, parentTransform, _transform);
        result.setRotation(_transform.getRotation());
    });
    return result;
}

const Transform SpatiallyNestable::getTransform() const {
    bool success;
    Transform result = getTransform(success);
    if (!success) {
        // There is a known issue related to child entities not being deleted
        // when their parent is removed. This has the side-effect that the
        // logs will be spammed with the following message. Until this is
        // fixed, this log message will be suppressed.
        //qCDebug(shared) << "getTransform failed for" << getID();
    }
    return result;
}

void SpatiallyNestable::breakParentingLoop() const {
    // someone created a loop.  break it...
    qCDebug(shared) << "Parenting loop detected: " << getID();
    SpatiallyNestablePointer _this = getThisPointer();
    _this->setParentID(QUuid());
    bool setPositionSuccess;
    AACube aaCube = getQueryAACube(setPositionSuccess);
    if (setPositionSuccess) {
        _this->setWorldPosition(aaCube.calcCenter());
    }
}

const Transform SpatiallyNestable::getJointTransform(int jointIndex, bool& success, int depth) const {
    // this returns the world-space transform for this object.  It finds its parent's transform (which may
    // cause this object's parent to query its parent, etc) and multiplies this object's local transform onto it.
    Transform jointInWorldFrame;

    if (depth > MAX_PARENTING_CHAIN_SIZE) {
        success = false;
        breakParentingLoop();
        return jointInWorldFrame;
    }

    Transform worldTransform = getTransform(success, depth);
    if (!success) {
        return jointInWorldFrame;
    }

    Transform jointInObjectFrame = getAbsoluteJointTransformInObjectFrame(jointIndex);
    Transform::mult(jointInWorldFrame, worldTransform, jointInObjectFrame);
    success = true;
    return jointInWorldFrame;
}

void SpatiallyNestable::setTransform(const Transform& transform, bool& success) {
    if (transform.containsNaN()) {
        success = false;
        return;
    }

    Transform parentTransform = getParentTransform(success);
    if (success) {
        bool changed = false;
        _transformLock.withWriteLock([&] {
            Transform beforeTransform = _transform;
            Transform::inverseMult(_transform, parentTransform, transform);
            if (_transform != beforeTransform) {
                changed = true;
                _translationChanged = usecTimestampNow();
                _rotationChanged = usecTimestampNow();
            }
        });
        if (changed) {
            locationChanged();
        }
    }
}

bool SpatiallyNestable::setTransform(const Transform& transform) {
    bool success;
    setTransform(transform, success);
    return success;
}

glm::vec3 SpatiallyNestable::getSNScale() const {
    bool success;
    auto result = getSNScale(success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- getScale failed" << getID();
    }
    #endif
    return result;
}

glm::vec3 SpatiallyNestable::getSNScale(bool& success) const {
    return getTransform(success).getScale();
}

glm::vec3 SpatiallyNestable::getJointSNScale(int jointIndex, bool& success) const {
    return getJointTransform(jointIndex, success).getScale();
}

void SpatiallyNestable::setSNScale(const glm::vec3& scale) {
    bool success;
    setSNScale(scale, success);
}

void SpatiallyNestable::setSNScale(const glm::vec3& scale, bool& success) {
    // guard against introducing NaN into the transform
    if (isNaN(scale)) {
        success = false;
        return;
    }

    bool changed = false;
    Transform parentTransform = getParentTransform(success);
    Transform myWorldTransform;
    _transformLock.withWriteLock([&] {
        Transform::mult(myWorldTransform, parentTransform, _transform);
        if (myWorldTransform.getScale() != scale) {
            changed = true;
            myWorldTransform.setScale(scale);
            Transform::inverseMult(_transform, parentTransform, myWorldTransform);
            _scaleChanged = usecTimestampNow();
        }
    });
    if (success && changed) {
        dimensionsChanged();
    }
}

Transform SpatiallyNestable::getLocalTransform() const {
    Transform result;
    _transformLock.withReadLock([&] {
        result =_transform;
    });
    return result;
}

void SpatiallyNestable::setLocalTransform(const Transform& transform) {
    // guard against introducing NaN into the transform
    if (transform.containsNaN()) {
        qCDebug(shared) << "SpatiallyNestable::setLocalTransform -- transform contains NaN";
        return;
    }

    bool changed = false;
    _transformLock.withWriteLock([&] {
        if (_transform != transform) {
            _transform = transform;
            changed = true;
            _scaleChanged = usecTimestampNow();
            _translationChanged = usecTimestampNow();
            _rotationChanged = usecTimestampNow();
        }
    });

    if (changed) {
        locationChanged();
    }
}

glm::vec3 SpatiallyNestable::getLocalPosition() const {
    glm::vec3 result;
    _transformLock.withReadLock([&] {
        result = _transform.getTranslation();
    });
    return result;
}

void SpatiallyNestable::setLocalPosition(const glm::vec3& position, bool tellPhysics) {
    // guard against introducing NaN into the transform
    if (isNaN(position)) {
        qCDebug(shared) << "SpatiallyNestable::setLocalPosition -- position contains NaN";
        return;
    }
    bool changed = false;
    _transformLock.withWriteLock([&] {
        if (_transform.getTranslation() != position) {
            _transform.setTranslation(position);
            changed = true;
            _translationChanged = usecTimestampNow();
        }
    });
    if (changed) {
        locationChanged(tellPhysics);
    }
}

glm::quat SpatiallyNestable::getLocalOrientation() const {
    glm::quat result;
    _transformLock.withReadLock([&] {
        result = _transform.getRotation();
    });
    return result;
}

void SpatiallyNestable::setLocalOrientation(const glm::quat& orientation) {
    // guard against introducing NaN into the transform
    if (isNaN(orientation)) {
        qCDebug(shared) << "SpatiallyNestable::setLocalOrientation -- orientation contains NaN";
        return;
    }
    bool changed = false;
    _transformLock.withWriteLock([&] {
        if (_transform.getRotation() != orientation) {
            _transform.setRotation(orientation);
            changed = true;
            _rotationChanged = usecTimestampNow();
        }
    });
    if (changed) {
        locationChanged();
    }
}

glm::vec3 SpatiallyNestable::getLocalVelocity() const {
    glm::vec3 result;
    _velocityLock.withReadLock([&] {
        result = _velocity;
    });
    return result;
}

void SpatiallyNestable::setLocalVelocity(const glm::vec3& velocity) {
    _velocityLock.withWriteLock([&] {
        _velocity = velocity;
    });
}

glm::vec3 SpatiallyNestable::getLocalAngularVelocity() const {
    glm::vec3 result;
    _angularVelocityLock.withReadLock([&] {
        result = _angularVelocity;
    });
    return result;
}

void SpatiallyNestable::setLocalAngularVelocity(const glm::vec3& angularVelocity) {
    _angularVelocityLock.withWriteLock([&] {
        _angularVelocity = angularVelocity;
    });
}

glm::vec3 SpatiallyNestable::getLocalSNScale() const {
    glm::vec3 result;
    _transformLock.withReadLock([&] {
        result = _transform.getScale();
    });
    return result;
}

void SpatiallyNestable::setLocalSNScale(const glm::vec3& scale) {
    // guard against introducing NaN into the transform
    if (isNaN(scale)) {
        qCDebug(shared) << "SpatiallyNestable::setLocalScale -- scale contains NaN";
        return;
    }

    bool changed = false;
    _transformLock.withWriteLock([&] {
        if (_transform.getScale() != scale) {
            _transform.setScale(scale);
            changed = true;
            _scaleChanged = usecTimestampNow();
        }
    });
    if (changed) {
        dimensionsChanged();
    }
}

QList<SpatiallyNestablePointer> SpatiallyNestable::getChildren() const {
    QList<SpatiallyNestablePointer> children;
    _childrenLock.withReadLock([&] {
        foreach(SpatiallyNestableWeakPointer childWP, _children.values()) {
            SpatiallyNestablePointer child = childWP.lock();
            // An object can set MyAvatar to be its parent using two IDs: the session ID and the special AVATAR_SELF_ID
            // Because we only recognize an object as having one ID, we need to check for the second possible ID here.
            // In practice, the AVATAR_SELF_ID should only be used for local-only objects.
            if (child && child->_parentKnowsMe && (child->getParentID() == getID() ||
                    (getNestableType() == NestableType::Avatar && child->getParentID() == AVATAR_SELF_ID))) {
                children << child;
            }
        }
    });
    return children;
}

bool SpatiallyNestable::hasChildren() const {
    bool result = false;
    _childrenLock.withReadLock([&] {
        if (_children.size() > 0) {
            result = true;
        }
    });
    return result;
}

const Transform SpatiallyNestable::getAbsoluteJointTransformInObjectFrame(int jointIndex) const {
    Transform jointTransformInObjectFrame;
    glm::vec3 position = getAbsoluteJointTranslationInObjectFrame(jointIndex);
    glm::quat orientation = getAbsoluteJointRotationInObjectFrame(jointIndex);
    glm::vec3 scale = getAbsoluteJointScaleInObjectFrame(jointIndex);
    jointTransformInObjectFrame.setScale(scale);
    jointTransformInObjectFrame.setRotation(orientation);
    jointTransformInObjectFrame.setTranslation(position);
    return jointTransformInObjectFrame;
}

SpatiallyNestablePointer SpatiallyNestable::getThisPointer() const {
    SpatiallyNestableConstPointer constThisPointer = shared_from_this();
    SpatiallyNestablePointer thisPointer = std::const_pointer_cast<SpatiallyNestable>(constThisPointer); // ermahgerd !!!
    return thisPointer;
}


void SpatiallyNestable::forEachChild(const ChildLambda& actor) const {
    for (auto& child : getChildren()) {
        actor(child);
    }
}

void SpatiallyNestable::forEachChildTest(const ChildLambdaTest&  actor) const {
    for (auto& child : getChildren()) {
        if (!actor(child)) {
            break;
        }
    }
}

// FIXME make a breadth_first_recursive_iterator to do this
void SpatiallyNestable::forEachDescendant(const ChildLambda& actor) const {
    std::list<SpatiallyNestablePointer> toProcess;
    {
        auto children = getChildren();
        toProcess.insert(toProcess.end(), children.begin(), children.end());
    }

    while (!toProcess.empty()) {
        auto& object = toProcess.front();
        actor(object);
        auto children = object->getChildren();
        toProcess.insert(toProcess.end(), children.begin(), children.end());
        toProcess.pop_front();
    }
}

void SpatiallyNestable::forEachDescendantTest(const ChildLambdaTest& actor) const {
    std::list<SpatiallyNestablePointer> toProcess;
    {
        auto children = getChildren();
        toProcess.insert(toProcess.end(), children.begin(), children.end());
    }

    while (!toProcess.empty()) {
        auto& object = toProcess.front();
        if (!actor(object)) {
            break;
        }
        auto children = object->getChildren();
        toProcess.insert(toProcess.end(), children.begin(), children.end());
        toProcess.pop_front();
    }
}

void SpatiallyNestable::locationChanged(bool tellPhysics, bool tellChildren) {
    if (tellChildren) {
        forEachChild([&](SpatiallyNestablePointer object) {
            object->locationChanged(tellPhysics, tellChildren);
        });
    }
}

AACube SpatiallyNestable::getMaximumAACube(bool& success) const {
    return AACube(getWorldPosition(success) - glm::vec3(defaultAACubeSize / 2.0f), defaultAACubeSize);
}

AACube SpatiallyNestable::calculateInitialQueryAACube(bool& success) {
    success = false;
    AACube maxAACube = getMaximumAACube(success);
    if (!success) {
        return AACube();
    }

    success = true;
    if (shouldPuffQueryAACube()) {
        // make an expanded AACube centered on the object
        const float PARENTED_EXPANSION_FACTOR = 3.0f;
        float scale = PARENTED_EXPANSION_FACTOR * maxAACube.getScale();
        return AACube(maxAACube.calcCenter() - glm::vec3(0.5f * scale), scale);
    } else {
        return maxAACube;
    }
}

bool SpatiallyNestable::updateQueryAACube(bool updateParent) {
    if (!queryAACubeNeedsUpdate()) {
        return false;
    }

    bool success;
    AACube initialQueryAACube = calculateInitialQueryAACube(success);
    if (!success) {
        return false;
    }
    _queryAACube = initialQueryAACube;
    _queryAACubeIsPuffed = shouldPuffQueryAACube();

    forEachDescendant([&](const SpatiallyNestablePointer& descendant) {
        bool childSuccess;
        AACube descendantAACube = descendant->getQueryAACube(childSuccess);
        if (childSuccess) {
            if (_queryAACube.contains(descendantAACube)) {
                return; // from lambda
            }
            _queryAACube += descendantAACube.getMinimumPoint();
            _queryAACube += descendantAACube.getMaximumPoint();
        }
    });

    _queryAACubeSet = true;

    if (updateParent) {
        auto parent = getParentPointer(success);
        if (success && parent) {
            parent->updateQueryAACube();
        }
    }

    return true;
}

bool SpatiallyNestable::updateQueryAACubeWithDescendantAACube(const AACube& descendantAACube, bool updateParent) {
    if (!queryAACubeNeedsUpdateWithDescendantAACube(descendantAACube)) {
        return false;
    }

    bool success;
    AACube initialQueryAACube = calculateInitialQueryAACube(success);
    if (!success) {
        return false;
    }
    _queryAACube = initialQueryAACube;
    _queryAACubeIsPuffed = shouldPuffQueryAACube();

    _queryAACube += descendantAACube.getMinimumPoint();
    _queryAACube += descendantAACube.getMaximumPoint();

    _queryAACubeSet = true;

    if (updateParent) {
        auto parent = getParentPointer(success);
        if (success && parent) {
            parent->updateQueryAACube();
        }
    }

    return true;
}

void SpatiallyNestable::setQueryAACube(const AACube& queryAACube) {
    if (queryAACube.containsNaN()) {
        qCDebug(shared) << "SpatiallyNestable::setQueryAACube -- cube contains NaN";
        return;
    }
    _queryAACube = queryAACube;
    _queryAACubeSet = true;
}

bool SpatiallyNestable::queryAACubeNeedsUpdate() const {
    if (!_queryAACubeSet) {
        return true;
    }

    bool success;
    AACube maxAACube = getMaximumAACube(success);
    if (success && !_queryAACube.contains(maxAACube)) {
        return true;
    }

    if (shouldPuffQueryAACube() != _queryAACubeIsPuffed) {
        return true;
    }

    // make sure children are still in their boxes, also.
    bool childNeedsUpdate = false;
    forEachDescendantTest([&](const SpatiallyNestablePointer& descendant) {
        if (descendant->queryAACubeNeedsUpdate() || !_queryAACube.contains(descendant->getQueryAACube())) {
            childNeedsUpdate = true;
            // Don't recurse further
            return false;
        }
        return true;
    });
    return childNeedsUpdate;
}

bool SpatiallyNestable::queryAACubeNeedsUpdateWithDescendantAACube(const AACube& descendantAACube) const {
    if (!_queryAACubeSet) {
        return true;
    }

    bool success;
    AACube maxAACube = getMaximumAACube(success);
    if (success && !_queryAACube.contains(maxAACube)) {
        return true;
    }

    if (shouldPuffQueryAACube() != _queryAACubeIsPuffed) {
        return true;
    }

    return !_queryAACube.contains(descendantAACube);
}

AACube SpatiallyNestable::getQueryAACube(bool& success) const {
    if (_queryAACubeSet) {
        success = true;
        return _queryAACube;
    }
    success = false;
    bool getPositionSuccess;
    return AACube(getWorldPosition(getPositionSuccess) - glm::vec3(defaultAACubeSize / 2.0f), defaultAACubeSize);
}

AACube SpatiallyNestable::getQueryAACube() const {
    bool success;
    auto result = getQueryAACube(success);
    if (!success) {
        qCDebug(shared) << "getQueryAACube failed for" << getID();
    }
    return result;
}

bool SpatiallyNestable::hasAncestorOfType(NestableType nestableType, int depth) const {
    if (depth > MAX_PARENTING_CHAIN_SIZE) {
        breakParentingLoop();
        return false;
    }

    if (nestableType == NestableType::Avatar) {
        QUuid parentID = getParentID();
        if (parentID == AVATAR_SELF_ID) {
            return true;
        }
    }

    bool success;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success || !parent) {
        return false;
    }

    if (parent->_nestableType == nestableType) {
        return true;
    }

    return parent->hasAncestorOfType(nestableType, depth + 1);
}

const QUuid SpatiallyNestable::findAncestorOfType(NestableType nestableType, int depth) const {
    if (depth > MAX_PARENTING_CHAIN_SIZE) {
        breakParentingLoop();
        return QUuid();
    }

    if (nestableType == NestableType::Avatar) {
        QUuid parentID = getParentID();
        if (parentID == AVATAR_SELF_ID) {
            return AVATAR_SELF_ID; // TODO -- can we put nodeID here?
        }
    }

    bool success;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success || !parent) {
        return QUuid();
    }

    if (parent->_nestableType == nestableType) {
        return parent->getID();
    }

    return parent->findAncestorOfType(nestableType, depth + 1);
}

void SpatiallyNestable::getLocalTransformAndVelocities(
        Transform& transform,
        glm::vec3& velocity,
        glm::vec3& angularVelocity) const {
    // transform
    _transformLock.withReadLock([&] {
        transform = _transform;
    });
    // linear velocity
    _velocityLock.withReadLock([&] {
        velocity = _velocity;
    });
    // angular velocity
    _angularVelocityLock.withReadLock([&] {
        angularVelocity = _angularVelocity;
    });
}

void SpatiallyNestable::setLocalTransformAndVelocities(
    const Transform& localTransform,
    const glm::vec3& localVelocity,
    const glm::vec3& localAngularVelocity) {

    bool changed = false;

    // transform
    _transformLock.withWriteLock([&] {
        if (_transform != localTransform) {
            _transform = localTransform;
            changed = true;
            _scaleChanged = usecTimestampNow();
            _translationChanged = usecTimestampNow();
            _rotationChanged = usecTimestampNow();
        }
    });
    // linear velocity
    _velocityLock.withWriteLock([&] {
        _velocity = localVelocity;
    });
    // angular velocity
    _angularVelocityLock.withWriteLock([&] {
        _angularVelocity = localAngularVelocity;
    });

    if (changed) {
        locationChanged(false);
    }
}

SpatiallyNestablePointer SpatiallyNestable::findByID(QUuid id, bool& success) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        return nullptr;
    }
    SpatiallyNestableWeakPointer parentWP = parentFinder->find(id, success);
    if (!success) {
        return nullptr;
    }
    return parentWP.lock();
}

/*@jsdoc
 * <p>An in-world item may be one of the following types:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"entity"</code></td><td>The item is an entity.</td></tr>
 *     <tr><td><code>"avatar"</code></td><td>The item is an avatar.</td></tr>
 *     <tr><td><code>"unknown"</code></td><td>The item cannot be found.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.NestableType
 */
QString SpatiallyNestable::nestableTypeToString(NestableType nestableType) {
    switch(nestableType) {
        case NestableType::Entity:
            return "entity";
        case NestableType::Avatar:
            return "avatar";
        default:
            return "unknown";
    }
}

void SpatiallyNestable::dump(const QString& prefix) const {
    qDebug().noquote() << prefix << "id = " << getID();
    qDebug().noquote() << prefix << "transform = " << _transform;
    bool success;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (success && parent) {
        parent->dump(prefix + "    ");
    }
}

bool SpatiallyNestable::isParentPathComplete(int depth) const {
    if (depth > MAX_PARENTING_CHAIN_SIZE) {
        breakParentingLoop();
        return false;
    }

    static const QUuid IDENTITY;
    QUuid parentID = getParentID();
    if (parentID.isNull() || parentID == IDENTITY) {
        return true;
    }

    bool success = false;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success || !parent) {
        return false;
    }

    return parent->isParentPathComplete(depth + 1);
}

void SpatiallyNestable::addGrab(GrabPointer grab) {
    _grabsLock.withWriteLock([&] {
        _grabs.insert(grab);
    });
}

void SpatiallyNestable::removeGrab(GrabPointer grab) {
    _grabsLock.withWriteLock([&] {
        _grabs.remove(grab);
    });
}

bool SpatiallyNestable::hasGrabs() {
    bool result { false };
    _grabsLock.withReadLock([&] {
        foreach (const GrabPointer &grab, _grabs) {
            if (grab && !grab->getReleased()) {
                result = true;
                break;
            }
        }
    });
    return result;
}

QUuid SpatiallyNestable::getEditSenderID() {
    // if more than one avatar is grabbing something, decide which one should tell the enity-server about it
    QUuid editSenderID;
    bool editSenderIDSet { false };
    _grabsLock.withReadLock([&] {
        foreach (const GrabPointer &grab, _grabs) {
            QUuid ownerID = grab->getOwnerID();
            if (!editSenderIDSet) {
                editSenderID = ownerID;
                editSenderIDSet = true;
            } else {
                if (ownerID < editSenderID) {
                    editSenderID = ownerID;
                }
            }
        }
    });
    return editSenderID;
}

void SpatiallyNestable::bumpAncestorChainRenderableVersion(int depth) const {
    if (depth > MAX_PARENTING_CHAIN_SIZE) {
        // can't break the parent chain here, because it will call setParentID, which calls this
        return;
    }

    _ancestorChainRenderableVersion++;
    bool success = false;
    auto parent = getParentPointer(success);
    if (success && parent) {
        parent->bumpAncestorChainRenderableVersion(depth + 1);
    }
}
