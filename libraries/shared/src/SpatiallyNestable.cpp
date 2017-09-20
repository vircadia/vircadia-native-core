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

#include <queue>

#include "DependencyManager.h"
#include "SharedUtil.h"
#include "StreamUtils.h"
#include "SharedLogging.h"

const float defaultAACubeSize = 1.0f;
const int maxParentingChain = 30;

SpatiallyNestable::SpatiallyNestable(NestableType nestableType, QUuid id) :
    _nestableType(nestableType),
    _id(id),
    _transform() {
    // set flags in _transform
    _transform.setTranslation(glm::vec3(0.0f));
    _transform.setRotation(glm::quat());
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
    _idLock.withWriteLock([&] {
        if (_parentID != parentID) {
            _parentID = parentID;
            _parentKnowsMe = false;
        }
    });

    bool success = false;
    getParentPointer(success);
}

Transform SpatiallyNestable::getParentTransform(bool& success, int depth) const {
    Transform result;
    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success) {
        return result;
    }
    if (parent) {
        result = parent->getTransform(_parentJointIndex, success, depth + 1);
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

    if (parent && parent->getID() == parentID) {
        // parent pointer is up-to-date
        if (!_parentKnowsMe) {
            parent->beParentOfChild(getThisPointer());
            _parentKnowsMe = true;
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
        parent->beParentOfChild(getThisPointer());
        _parentKnowsMe = true;
    }

    success = (parent || parentID.isNull());
    return parent;
}

void SpatiallyNestable::beParentOfChild(SpatiallyNestablePointer newChild) const {
    _childrenLock.withWriteLock([&] {
        _children[newChild->getID()] = newChild;
    });
}

void SpatiallyNestable::forgetChild(SpatiallyNestablePointer newChild) const {
    _childrenLock.withWriteLock([&] {
        _children.remove(newChild->getID());
    });
}

void SpatiallyNestable::setParentJointIndex(quint16 parentJointIndex) {
    _parentJointIndex = parentJointIndex;
}

glm::vec3 SpatiallyNestable::worldToLocal(const glm::vec3& position,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool& success) {
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
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::vec3(0.0f);
        }
    }
    success = true;

    Transform invParentTransform;
    parentTransform.evalInverse(invParentTransform);
    return invParentTransform.transform(position);
}

glm::quat SpatiallyNestable::worldToLocal(const glm::quat& orientation,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool& success) {
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
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::quat();
        }
    }
    success = true;

    glm::quat invParentOrientation = glm::inverse(parentTransform.getRotation());
    return invParentOrientation * orientation;
}

glm::vec3 SpatiallyNestable::worldToLocalVelocity(const glm::vec3& velocity, const QUuid& parentID,
                                                  int parentJointIndex, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return velocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return velocity;
    }
    glm::vec3 parentVelocity = parent->getVelocity(success);
    if (!success) {
        return velocity;
    }

    return glm::inverse(parentTransform.getRotation()) * (velocity - parentVelocity);
}

glm::vec3 SpatiallyNestable::worldToLocalAngularVelocity(const glm::vec3& angularVelocity, const QUuid& parentID,
                                                         int parentJointIndex, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return angularVelocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return angularVelocity;
    }

    return glm::inverse(parentTransform.getRotation()) * angularVelocity;
}

glm::vec3 SpatiallyNestable::localToWorld(const glm::vec3& position,
                                          const QUuid& parentID, int parentJointIndex,
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
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::vec3(0.0f);
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
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::quat();
        }
        parentTransform.setScale(1.0f);
    }
    success = true;

    Transform orientationTransform;
    orientationTransform.setRotation(orientation);
    Transform::mult(result, parentTransform, orientationTransform);
    return result.getRotation();
}

glm::vec3 SpatiallyNestable::localToWorldVelocity(const glm::vec3& velocity, const QUuid& parentID,
                                                  int parentJointIndex, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return velocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return velocity;
    }
    glm::vec3 parentVelocity = parent->getVelocity(success);
    if (!success) {
        return velocity;
    }

    return parentVelocity + parentTransform.getRotation() * velocity;
}

glm::vec3 SpatiallyNestable::localToWorldAngularVelocity(const glm::vec3& angularVelocity, const QUuid& parentID,
                                                  int parentJointIndex, bool& success) {
    SpatiallyNestablePointer parent = SpatiallyNestable::findByID(parentID, success);
    if (!success || !parent) {
        return angularVelocity;
    }
    Transform parentTransform = parent->getTransform(success);
    if (!success) {
        return angularVelocity;
    }

    return parentTransform.getRotation() * angularVelocity;
}

glm::vec3 SpatiallyNestable::getPosition(bool& success) const {
    return getTransform(success).getTranslation();
}

glm::vec3 SpatiallyNestable::getPosition() const {
    bool success;
    auto result = getPosition(success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- getPosition failed" << getID();
    }
    #endif
    return result;
}

glm::vec3 SpatiallyNestable::getPosition(int jointIndex, bool& success) const {
    return getTransform(jointIndex, success).getTranslation();
}

void SpatiallyNestable::setPosition(const glm::vec3& position, bool& success, bool tellPhysics) {
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

void SpatiallyNestable::setPosition(const glm::vec3& position) {
    bool success;
    setPosition(position, success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- setPosition failed" << getID();
    }
    #endif
}

glm::quat SpatiallyNestable::getOrientation(bool& success) const {
    return getTransform(success).getRotation();
}

glm::quat SpatiallyNestable::getOrientation() const {
    bool success;
    auto result = getOrientation(success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- getOrientation failed" << getID();
    }
    #endif
    return result;
}

glm::quat SpatiallyNestable::getOrientation(int jointIndex, bool& success) const {
    return getTransform(jointIndex, success).getRotation();
}

void SpatiallyNestable::setOrientation(const glm::quat& orientation, bool& success, bool tellPhysics) {
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

void SpatiallyNestable::setOrientation(const glm::quat& orientation) {
    bool success;
    setOrientation(orientation, success);
    #ifdef WANT_DEBUG
    if (!success) {
        qCDebug(shared) << "Warning -- setOrientation failed" << getID();
    }
    #endif
}

glm::vec3 SpatiallyNestable::getVelocity(bool& success) const {
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

glm::vec3 SpatiallyNestable::getVelocity() const {
    bool success;
    glm::vec3 result = getVelocity(success);
    if (!success) {
        qCDebug(shared) << "Warning -- getVelocity failed" << getID();
    }
    return result;
}

void SpatiallyNestable::setVelocity(const glm::vec3& velocity, bool& success) {
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

void SpatiallyNestable::setVelocity(const glm::vec3& velocity) {
    bool success;
    setVelocity(velocity, success);
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
        result = parent->getVelocity(success);
    }
    return result;
}

glm::vec3 SpatiallyNestable::getAngularVelocity(bool& success) const {
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

glm::vec3 SpatiallyNestable::getAngularVelocity() const {
    bool success;
    glm::vec3 result = getAngularVelocity(success);
    if (!success) {
        qCDebug(shared) << "Warning -- getAngularVelocity failed" << getID();
    }
    return result;
}

void SpatiallyNestable::setAngularVelocity(const glm::vec3& angularVelocity, bool& success) {
    glm::vec3 parentAngularVelocity = getParentAngularVelocity(success);
    Transform parentTransform = getParentTransform(success);
    _angularVelocityLock.withWriteLock([&] {
        _angularVelocity = glm::inverse(parentTransform.getRotation()) * (angularVelocity - parentAngularVelocity);
    });
}

void SpatiallyNestable::setAngularVelocity(const glm::vec3& angularVelocity) {
    bool success;
    setAngularVelocity(angularVelocity, success);
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
        result = parent->getAngularVelocity(success);
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

const Transform SpatiallyNestable::getTransform() const {
    bool success;
    Transform result = getTransform(success);
    if (!success) {
        qCDebug(shared) << "getTransform failed for" << getID();
    }
    return result;
}

const Transform SpatiallyNestable::getTransform(int jointIndex, bool& success, int depth) const {
    // this returns the world-space transform for this object.  It finds its parent's transform (which may
    // cause this object's parent to query its parent, etc) and multiplies this object's local transform onto it.
    Transform jointInWorldFrame;

    if (depth > maxParentingChain) {
        success = false;
        // someone created a loop.  break it...
        qCDebug(shared) << "Parenting loop detected.";
        SpatiallyNestablePointer _this = getThisPointer();
        _this->setParentID(QUuid());
        bool setPositionSuccess;
        AACube aaCube = getQueryAACube(setPositionSuccess);
        if (setPositionSuccess) {
            _this->setPosition(aaCube.calcCenter());
        }
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

    bool changed = false;
    Transform parentTransform = getParentTransform(success);
    _transformLock.withWriteLock([&] {
        Transform beforeTransform = _transform;
        Transform::inverseMult(_transform, parentTransform, transform);
        if (_transform != beforeTransform) {
            changed = true;
            _translationChanged = usecTimestampNow();
            _rotationChanged = usecTimestampNow();
        }
    });
    if (success && changed) {
        locationChanged();
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

glm::vec3 SpatiallyNestable::getSNScale(int jointIndex, bool& success) const {
    return getTransform(jointIndex, success).getScale();
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
        locationChanged();
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

void SpatiallyNestable::locationChanged(bool tellPhysics) {
    forEachChild([&](SpatiallyNestablePointer object) {
        object->locationChanged(tellPhysics);
    });
}

AACube SpatiallyNestable::getMaximumAACube(bool& success) const {
    return AACube(getPosition(success) - glm::vec3(defaultAACubeSize / 2.0f), defaultAACubeSize);
}

const float PARENTED_EXPANSION_FACTOR = 3.0f;

bool SpatiallyNestable::checkAndMaybeUpdateQueryAACube() {
    bool success = false;
    AACube maxAACube = getMaximumAACube(success);
    if (success) {
        // maybe update _queryAACube
        if (!_queryAACubeSet || (_parentID.isNull() && _children.size() == 0) || !_queryAACube.contains(maxAACube)) {
            if (_parentJointIndex != INVALID_JOINT_INDEX || _children.size() > 0 ) {
                // make an expanded AACube centered on the object
                float scale = PARENTED_EXPANSION_FACTOR * maxAACube.getScale();
                _queryAACube = AACube(maxAACube.calcCenter() - glm::vec3(0.5f * scale), scale);
            } else {
                _queryAACube = maxAACube;
            }

            forEachDescendant([&](const SpatiallyNestablePointer& descendant) {
                bool childSuccess;
                AACube descendantAACube = descendant->getQueryAACube(childSuccess);
                if (childSuccess) {
                    if (_queryAACube.contains(descendantAACube)) {
                        return ;
                    }
                    _queryAACube += descendantAACube.getMinimumPoint();
                    _queryAACube += descendantAACube.getMaximumPoint();
                }
            });
            _queryAACubeSet = true;
        }
    }
    return success;
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

    // make sure children are still in their boxes, also.
    bool childNeedsUpdate = false;
    forEachDescendantTest([&](const SpatiallyNestablePointer& descendant) {
        if (!childNeedsUpdate && descendant->queryAACubeNeedsUpdate()) {
            childNeedsUpdate = true;
            // Don't recurse further
            return false;
        }
        return true;
    });
    return childNeedsUpdate;
}

void SpatiallyNestable::updateQueryAACube() {
    bool success;
    AACube maxAACube = getMaximumAACube(success);
    if (_parentJointIndex != INVALID_JOINT_INDEX || _children.size() > 0 ) {
        // make an expanded AACube centered on the object
        float scale = PARENTED_EXPANSION_FACTOR * maxAACube.getScale();
        _queryAACube = AACube(maxAACube.calcCenter() - glm::vec3(0.5f * scale), scale);
    } else {
        _queryAACube = maxAACube;
    }

    forEachDescendant([&](const SpatiallyNestablePointer& descendant) {
        bool success;
        AACube descendantAACube = descendant->getQueryAACube(success);
        if (success) {
            if (_queryAACube.contains(descendantAACube)) {
                return;
            }
            _queryAACube += descendantAACube.getMinimumPoint();
            _queryAACube += descendantAACube.getMaximumPoint();
        }
    });
    _queryAACubeSet = true;
}

AACube SpatiallyNestable::getQueryAACube(bool& success) const {
    if (_queryAACubeSet) {
        success = true;
        return _queryAACube;
    }
    success = false;
    bool getPositionSuccess;
    return AACube(getPosition(getPositionSuccess) - glm::vec3(defaultAACubeSize / 2.0f), defaultAACubeSize);
}

AACube SpatiallyNestable::getQueryAACube() const {
    bool success;
    auto result = getQueryAACube(success);
    if (!success) {
        qCDebug(shared) << "getQueryAACube failed for" << getID();
    }
    return result;
}

bool SpatiallyNestable::hasAncestorOfType(NestableType nestableType) const {
    bool success;
    if (nestableType == NestableType::Avatar) {
        QUuid parentID = getParentID();
        if (parentID == AVATAR_SELF_ID) {
            return true;
        }
    }

    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success || !parent) {
        return false;
    }

    if (parent->_nestableType == nestableType) {
        return true;
    }

    return parent->hasAncestorOfType(nestableType);
}

const QUuid SpatiallyNestable::findAncestorOfType(NestableType nestableType) const {
    bool success;

    if (nestableType == NestableType::Avatar) {
        QUuid parentID = getParentID();
        if (parentID == AVATAR_SELF_ID) {
            return AVATAR_SELF_ID; // TODO -- can we put nodeID here?
        }
    }

    SpatiallyNestablePointer parent = getParentPointer(success);
    if (!success || !parent) {
        return QUuid();
    }

    if (parent->_nestableType == nestableType) {
        return parent->getID();
    }

    return parent->findAncestorOfType(nestableType);
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


QString SpatiallyNestable::nestableTypeToString(NestableType nestableType) {
    switch(nestableType) {
        case NestableType::Entity:
            return "entity";
        case NestableType::Avatar:
            return "avatar";
        case NestableType::Overlay:
            return "overlay";
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
