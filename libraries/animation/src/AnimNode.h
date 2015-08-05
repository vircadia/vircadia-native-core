//
//  AnimNode.h
//
//  Copyright 2015 High Fidelity, Inc.
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

class QJsonObject;

class AnimNode {
public:
    friend class AnimDebugDraw;

    enum Type {
        ClipType = 0,
        BlendLinearType,
        NumTypes
    };
    typedef std::shared_ptr<AnimNode> Pointer;

    AnimNode(Type type, const std::string& id) : _type(type), _id(id) {}

    const std::string& getID() const { return _id; }
    Type getType() const { return _type; }

    void addChild(Pointer child) { _children.push_back(child); }
    void removeChild(Pointer child) {
        auto iter = std::find(_children.begin(), _children.end(), child);
        if (iter != _children.end()) {
            _children.erase(iter);
        }
    }
    Pointer getChild(int i) const {
        assert(i >= 0 && i < (int)_children.size());
        return _children[i];
    }
    int getChildCount() const { return (int)_children.size(); }

    void setSkeleton(AnimSkeleton::Pointer skeleton) {
        setSkeletonInternal(skeleton);
        for (auto&& child : _children) {
            child->setSkeletonInternal(skeleton);
        }
    }

    AnimSkeleton::Pointer getSkeleton() const { return _skeleton; }

    virtual ~AnimNode() {}

    virtual const std::vector<AnimPose>& evaluate(float dt) = 0;

protected:

    virtual void setSkeletonInternal(AnimSkeleton::Pointer skeleton) {
        _skeleton = skeleton;
    }

    // for AnimDebugDraw rendering
    virtual const std::vector<AnimPose>& getPosesInternal() const = 0;

    Type _type;
    std::string _id;
    std::vector<AnimNode::Pointer> _children;
    AnimSkeleton::Pointer _skeleton;

    // no copies
    AnimNode(const AnimNode&) = delete;
    AnimNode& operator=(const AnimNode&) = delete;
};

#endif // hifi_AnimNode_h
