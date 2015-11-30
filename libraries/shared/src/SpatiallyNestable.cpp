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

#include "DependencyManager.h"
#include "SpatiallyNestable.h"


SpatiallyNestable::SpatiallyNestable(NestableTypes::NestableType nestableType, QUuid id) :
    _nestableType(nestableType),
    _id(id),
    _transform() {
    // set flags in _transform
    _transform.setTranslation(glm::vec3(0.0f));
    _transform.setRotation(glm::quat());
}

Transform SpatiallyNestable::getParentTransform() const {
    Transform result;
    SpatiallyNestablePointer parent = getParentPointer();
    if (parent) {
        Transform parentTransform = parent->getTransform(_parentJointIndex);
        result = parentTransform.setScale(1.0f);
    }
    return result;
}

SpatiallyNestablePointer SpatiallyNestable::getParentPointer() const {
    SpatiallyNestablePointer parent = _parent.lock();

    if (!parent && _parentID.isNull()) {
        // no parent
        return nullptr;
    }

    SpatiallyNestableConstPointer constThisPointer = shared_from_this();
    SpatiallyNestablePointer thisPointer = std::const_pointer_cast<SpatiallyNestable>(constThisPointer); // ermahgerd !!!

    if (parent && parent->getID() == _parentID) {
        // parent pointer is up-to-date
        if (!_parentKnowsMe) {
            parent->beParentOfChild(thisPointer);
            _parentKnowsMe = true;
        }
        return parent;
    }

    if (parent) {
        // we have a parent pointer but our _parentID doesn't indicate this parent.
        parent->forgetChild(thisPointer);
        _parentKnowsMe = false;
        _parent.reset();
    }

    // we have a _parentID but no parent pointer, or our parent pointer was to the wrong thing
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    _parent = parentFinder->find(_parentID);
    parent = _parent.lock();
    if (parent) {
        parent->beParentOfChild(thisPointer);
        _parentKnowsMe = true;
    }

    if (parent || _parentID.isNull()) {
        thisPointer->parentChanged();
    }

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

void SpatiallyNestable::setParentID(const QUuid parentID) {
    if (_parentID != parentID) {
        _parentID = parentID;
        _parentKnowsMe = false;
    }
}

glm::vec3 SpatiallyNestable::worldToLocal(const glm::vec3 position, QUuid parentID, int parentJointIndex) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    auto parentWP = parentFinder->find(parentID);
    auto parent = parentWP.lock();
    Transform parentTransform;
    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex);
        parentTransform.setScale(1.0f);
    }

    Transform positionTransform;
    positionTransform.setTranslation(position);
    Transform myWorldTransform;
    Transform::mult(myWorldTransform, parentTransform, positionTransform);

    myWorldTransform.setTranslation(position);
    Transform result;
    Transform::inverseMult(result, parentTransform, myWorldTransform);
    return result.getTranslation();
}

glm::quat SpatiallyNestable::worldToLocal(const glm::quat orientation, QUuid parentID, int parentJointIndex) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    auto parentWP = parentFinder->find(parentID);
    auto parent = parentWP.lock();
    Transform parentTransform;
    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex);
        parentTransform.setScale(1.0f);
    }

    Transform orientationTransform;
    orientationTransform.setRotation(orientation);
    Transform myWorldTransform;
    Transform::mult(myWorldTransform, parentTransform, orientationTransform);
    myWorldTransform.setRotation(orientation);
    Transform result;
    Transform::inverseMult(result, parentTransform, myWorldTransform);
    return result.getRotation();
}

glm::vec3 SpatiallyNestable::localToWorld(const glm::vec3 position, QUuid parentID, int parentJointIndex) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    auto parentWP = parentFinder->find(parentID);
    auto parent = parentWP.lock();
    Transform parentTransform;
    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex);
        parentTransform.setScale(1.0f);
    }
    Transform positionTransform;
    positionTransform.setTranslation(position);
    Transform result;
    Transform::mult(result, parentTransform, positionTransform);
    return result.getTranslation();
}

glm::quat SpatiallyNestable::localToWorld(const glm::quat orientation, QUuid parentID, int parentJointIndex) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    auto parentWP = parentFinder->find(parentID);
    auto parent = parentWP.lock();
    Transform parentTransform;
    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex);
        parentTransform.setScale(1.0f);
    }
    Transform orientationTransform;
    orientationTransform.setRotation(orientation);
    Transform result;
    Transform::mult(result, parentTransform, orientationTransform);
    return result.getRotation();
}


const glm::vec3 SpatiallyNestable::getPosition() const {
    Transform parentTransformDescaled = getParentTransform();
    glm::mat4 parentMat;
    parentTransformDescaled.getMatrix(parentMat);
    glm::vec4 absPos = parentMat * glm::vec4(getLocalPosition(), 1.0f);
    return glm::vec3(absPos);
}

const glm::vec3 SpatiallyNestable::getPosition(int jointIndex) const {
    Transform worldTransform = getTransform();
    Transform jointInObjectFrame = getJointTransformInObjectFrame(jointIndex);
    Transform jointInWorldFrame;
    Transform::mult(jointInWorldFrame, worldTransform, jointInObjectFrame);
    return jointInWorldFrame.getTranslation();
}

void SpatiallyNestable::setPosition(const glm::vec3 position) {
    Transform parentTransform = getParentTransform();
    Transform myWorldTransform;
    _transformLock.withWriteLock([&] {
        Transform::mult(myWorldTransform, parentTransform, _transform);
        myWorldTransform.setTranslation(position);
        Transform::inverseMult(_transform, parentTransform, myWorldTransform);
    });
}

const glm::quat SpatiallyNestable::getOrientation() const {
    Transform parentTransformDescaled = getParentTransform();
    return parentTransformDescaled.getRotation() * getLocalOrientation();
}

const glm::quat SpatiallyNestable::getOrientation(int jointIndex) const {
    Transform worldTransform = getTransform();
    Transform jointInObjectFrame = getJointTransformInObjectFrame(jointIndex);
    Transform jointInWorldFrame;
    Transform::mult(jointInWorldFrame, worldTransform, jointInObjectFrame);
    return jointInWorldFrame.getRotation();
}

void SpatiallyNestable::setOrientation(const glm::quat orientation) {
    Transform parentTransform = getParentTransform();
    Transform myWorldTransform;
    _transformLock.withWriteLock([&] {
        Transform::mult(myWorldTransform, parentTransform, _transform);
        myWorldTransform.setRotation(orientation);
        Transform::inverseMult(_transform, parentTransform, myWorldTransform);
    });
}

const Transform SpatiallyNestable::getTransform() const {
    Transform parentTransform = getParentTransform();
    Transform result;
    _transformLock.withReadLock([&] {
        Transform::mult(result, parentTransform, _transform);
    });
    return result;
}

const Transform SpatiallyNestable::getTransform(int jointIndex) const {
    Transform worldTransform = getTransform();
    Transform jointInObjectFrame = getJointTransformInObjectFrame(jointIndex);
    Transform jointInWorldFrame;
    Transform::mult(jointInWorldFrame, worldTransform, jointInObjectFrame);
    return jointInWorldFrame;
}

void SpatiallyNestable::setTransform(const Transform transform) {
    Transform parentTransform = getParentTransform();
    _transformLock.withWriteLock([&] {
        Transform::inverseMult(_transform, parentTransform, transform);
    });
}

const glm::vec3 SpatiallyNestable::getScale() const {
    glm::vec3 result;
    _transformLock.withReadLock([&] {
        result = _transform.getScale();
    });
    return result;
}

const glm::vec3 SpatiallyNestable::getScale(int jointIndex) const {
    // XXX ... something with joints
    return getScale();
}

void SpatiallyNestable::setScale(const glm::vec3 scale) {
    _transformLock.withWriteLock([&] {
        _transform.setScale(scale);
    });
}

const Transform SpatiallyNestable::getLocalTransform() const {
    Transform result;
    _transformLock.withReadLock([&] {
        result =_transform;
    });
    return result;
}

void SpatiallyNestable::setLocalTransform(const Transform transform) {
    _transformLock.withWriteLock([&] {
        _transform = transform;
    });
}

const glm::vec3 SpatiallyNestable::getLocalPosition() const {
    glm::vec3 result;
    _transformLock.withReadLock([&] {
        result = _transform.getTranslation();
    });
    return result;
}

void SpatiallyNestable::setLocalPosition(const glm::vec3 position) {
    _transformLock.withWriteLock([&] {
        _transform.setTranslation(position);
    });
}

const glm::quat SpatiallyNestable::getLocalOrientation() const {
    glm::quat result;
    _transformLock.withReadLock([&] {
        result = _transform.getRotation();
    });
    return result;
}

void SpatiallyNestable::setLocalOrientation(const glm::quat orientation) {
    _transformLock.withWriteLock([&] {
        _transform.setRotation(orientation);
    });
}

const glm::vec3 SpatiallyNestable::getLocalScale() const {
    glm::vec3 result;
    _transformLock.withReadLock([&] {
        result = _transform.getScale();
    });
    return result;
}

void SpatiallyNestable::setLocalScale(const glm::vec3 scale) {
    _transformLock.withWriteLock([&] {
        _transform.setScale(scale);
    });
}

QList<SpatiallyNestablePointer> SpatiallyNestable::getChildren() const {
    QList<SpatiallyNestablePointer> children;
    _childrenLock.withReadLock([&] {
        foreach (SpatiallyNestableWeakPointer childWP, _children.values()) {
            SpatiallyNestablePointer child = childWP.lock();
            if (child) {
                children << child;
            }
        }
    });
    return children;
}


const Transform SpatiallyNestable::getJointTransformInObjectFrame(int jointIndex) const {
    Transform jointInObjectFrame;
    glm::vec3 position = getJointTranslation(jointIndex);
    glm::quat orientation = getJointRotation(jointIndex);
    jointInObjectFrame.setRotation(orientation);
    jointInObjectFrame.setTranslation(position);
    return jointInObjectFrame;
}
