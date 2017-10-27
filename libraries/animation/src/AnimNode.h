//
//  AnimNode.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimNode_h
#define hifi_AnimNode_h

#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AnimSkeleton.h"
#include "AnimVariant.h"
#include "AnimContext.h"

class QJsonObject;

// Base class for all elements in the animation blend tree.
// It provides the following categories of functions:
//
//   * id getter, id is used to identify a node, useful for debugging and node searching.
//   * type getter, helpful for determining the derived type of this node.
//   * hierarchy accessors, for adding, removing and iterating over child AnimNodes
//   * skeleton accessors, the skeleton is from the model whose bones we are going to manipulate
//   * evaluate method, perform actual joint manipulations here and return result by reference.
//     Also, append any triggers that are detected during evaluation.

class AnimNode : public std::enable_shared_from_this<AnimNode> {
public:
    enum class Type {
        Clip = 0,
        BlendLinear,
        BlendLinearMove,
        Overlay,
        StateMachine,
        Manipulator,
        InverseKinematics,
        DefaultPose,
        NumTypes
    };
    using Pointer = std::shared_ptr<AnimNode>;
    using ConstPointer = std::shared_ptr<const AnimNode>;
    using Triggers = std::vector<QString>;

    friend class AnimDebugDraw;
    friend void buildChildMap(std::map<QString, Pointer>& map, Pointer node);
    friend class AnimStateMachine;

    AnimNode(Type type, const QString& id) : _type(type), _id(id) {}
    virtual ~AnimNode() {}

    const QString& getID() const { return _id; }
    Type getType() const { return _type; }

    // hierarchy accessors
    Pointer getParent();
    void addChild(Pointer child);
    void removeChild(Pointer child);
    void replaceChild(Pointer oldChild, Pointer newChild);
    Pointer getChild(int i) const;
    int getChildCount() const { return (int)_children.size(); }

    // pair this AnimNode graph with a skeleton.
    void setSkeleton(AnimSkeleton::ConstPointer skeleton);

    AnimSkeleton::ConstPointer getSkeleton() const { return _skeleton; }

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut) = 0;
    virtual const AnimPoseVec& overlay(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut,
                                       const AnimPoseVec& underPoses) {
        return evaluate(animVars, context, dt, triggersOut);
    }

    void setCurrentFrame(float frame);

    template <typename F>
    bool traverse(F func) {
        if (func(shared_from_this())) {
            for (auto&& child : _children) {
                if (!child->traverse(func)) {
                    return false;
                }
            }
        }
        return true;
    }

    Pointer findByName(const QString& id) {
        Pointer result;
        traverse([&](Pointer node) {
            if (id == node->getID()) {
                result = node;
                return false;
            }
            return true;
        });
        return result;
    }

protected:

    virtual void setCurrentFrameInternal(float frame) {}
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) { _skeleton = skeleton; }

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const = 0;

    Type _type;
    QString _id;
    std::vector<AnimNode::Pointer> _children;
    AnimSkeleton::ConstPointer _skeleton;
    std::weak_ptr<AnimNode> _parent;

    // no copies
    AnimNode(const AnimNode&) = delete;
    AnimNode& operator=(const AnimNode&) = delete;
};

#endif // hifi_AnimNode_h
