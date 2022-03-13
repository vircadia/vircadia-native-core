//
//  Created by Sabrina Shanman 2018/08/14
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_NestableTransformNode_h
#define hifi_NestableTransformNode_h

#include "TransformNode.h"

#include "SpatiallyNestable.h"

#include "RegisteredMetaTypes.h"

template <typename T>
class BaseNestableTransformNode : public TransformNode {
public:
    BaseNestableTransformNode(std::weak_ptr<T> spatiallyNestable, int jointIndex) :
        _spatiallyNestable(spatiallyNestable),
        _jointIndex(jointIndex) {
        auto nestablePointer = _spatiallyNestable.lock();
        if (nestablePointer) {
            if (nestablePointer->getNestableType() != NestableType::Avatar) {
                glm::vec3 nestableDimensions = getActualScale(nestablePointer);
                _baseScale = glm::max(glm::vec3(0.001f), nestableDimensions);
            }
        }
    }

    Transform getTransform() override {
        std::shared_ptr<T> nestable = _spatiallyNestable.lock();
        if (!nestable) {
            return Transform();
        }

        bool success;
        Transform jointWorldTransform = nestable->getJointTransform(_jointIndex, success);

        if (!success) {
            return Transform();
        }

        jointWorldTransform.setScale(getActualScale(nestable) / _baseScale);

        return jointWorldTransform;
    }

    QVariantMap toVariantMap() const override {
        QVariantMap map;

        auto nestable = _spatiallyNestable.lock();
        if (nestable) {
            map["parentID"] = nestable->getID();
            map["parentJointIndex"] = _jointIndex;
            map["baseParentScale"] = vec3toVariant(_baseScale);
        }

        return map;
    }

    glm::vec3 getActualScale(const std::shared_ptr<T>& nestablePointer) const;

protected:
    std::weak_ptr<T> _spatiallyNestable;
    int _jointIndex;
    glm::vec3 _baseScale { 1.0f };
};

class NestableTransformNode : public BaseNestableTransformNode<SpatiallyNestable> {
public:
    NestableTransformNode(std::weak_ptr<SpatiallyNestable> spatiallyNestable, int jointIndex) : BaseNestableTransformNode(spatiallyNestable, jointIndex) {};
};

#endif // hifi_NestableTransformNode_h
