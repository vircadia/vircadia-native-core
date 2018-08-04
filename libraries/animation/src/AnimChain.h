//
//  AnimChain.h
//
//  Created by Anthony J. Thibault on 7/16/2018.
//  Copyright (c) 2018 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimChain
#define hifi_AnimChain

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <DebugDraw.h>

template <int N>
class AnimChainT {

public:
    AnimChainT() {}

    AnimChainT(const AnimChainT& orig) {
        _top = orig._top;
        for (int i = 0; i < _top; i++) {
            _chain[i] = orig._chain[i];
        }
    }

    AnimChainT& operator=(const AnimChainT& orig) {
        _top = orig._top;
        for (int i = 0; i < _top; i++) {
            _chain[i] = orig._chain[i];
        }
        return *this;
    }

    bool buildFromRelativePoses(const AnimSkeleton::ConstPointer& skeleton, const AnimPoseVec& relativePoses, int tipIndex) {
        _top = 0;
        // iterate through the skeleton parents, from the tip to the base, copying over relativePoses into the chain.
        for (int jointIndex = tipIndex; jointIndex != -1; jointIndex = skeleton->getParentIndex(jointIndex)) {
            if (_top >= N) {
                assert(_top < N);
                // stack overflow
                return false;
            }
            _chain[_top].relativePose = relativePoses[jointIndex];
            _chain[_top].jointIndex = jointIndex;
            _chain[_top].dirty = true;
            _top++;
        }

        buildDirtyAbsolutePoses();

        return true;
    }

    const AnimPose& getAbsolutePoseFromJointIndex(int jointIndex) const {
        for (int i = 0; i < _top; i++) {
            if (_chain[i].jointIndex == jointIndex) {
                return _chain[i].absolutePose;
            }
        }
        return AnimPose::identity;
    }

    bool setRelativePoseAtJointIndex(int jointIndex, const AnimPose& relativePose) {
        bool foundIndex = false;
        for (int i = _top - 1; i >= 0; i--) {
            if (_chain[i].jointIndex == jointIndex) {
                _chain[i].relativePose = relativePose;
                foundIndex = true;
            }
            // all child absolute poses are now dirty
            if (foundIndex) {
                _chain[i].dirty = true;
            }
        }
        return foundIndex;
    }

    void buildDirtyAbsolutePoses() {
        // the relative and absolute pose is the same for the base of the chain.
        _chain[_top - 1].absolutePose = _chain[_top - 1].relativePose;
        _chain[_top - 1].dirty = false;

        // iterate chain from base to tip, concatinating the relative poses to build the absolute poses.
        for (int i = _top - 1; i > 0; i--) {
            AnimChainElem& parent = _chain[i];
            AnimChainElem& child = _chain[i - 1];

            if (child.dirty) {
                child.absolutePose = parent.absolutePose * child.relativePose;
                child.dirty = false;
            }
        }
    }

    void blend(const AnimChainT& srcChain, float alpha) {
        // make sure chains have same lengths
        assert(srcChain._top == _top);
        if (srcChain._top != _top) {
            return;
        }

        // only blend the relative poses
        for (int i = 0; i < _top; i++) {
            _chain[i].relativePose.blend(srcChain._chain[i].relativePose, alpha);
            _chain[i].dirty = true;
        }
    }

    int size() const {
        return _top;
    }

    void outputRelativePoses(AnimPoseVec& relativePoses) {
        for (int i = 0; i < _top; i++) {
            relativePoses[_chain[i].jointIndex] = _chain[i].relativePose;
        }
    }

    void debugDraw(const glm::mat4& geomToWorldMat, const glm::vec4& color) const {
        for (int i = 1; i < _top; i++) {
            glm::vec3 start = transformPoint(geomToWorldMat, _chain[i - 1].absolutePose.trans());
            glm::vec3 end = transformPoint(geomToWorldMat, _chain[i].absolutePose.trans());
            DebugDraw::getInstance().drawRay(start, end, color);
        }
    }

    void dump() const {
        for (int i = 0; i < _top; i++) {
            qWarning() << "AJT:    AnimPoseElem[" << i << "]";
            qWarning() << "AJT:        relPose =" << _chain[i].relativePose;
            qWarning() << "AJT:        absPose =" << _chain[i].absolutePose;
            qWarning() << "AJT:        jointIndex =" << _chain[i].jointIndex;
            qWarning() << "AJT:        dirty =" << _chain[i].dirty;
        }
    }

protected:

    struct AnimChainElem {
        AnimPose relativePose;
        AnimPose absolutePose;
        int jointIndex { -1 };
        bool dirty { true };
    };

    AnimChainElem _chain[N];
    int _top { 0 };
};

using AnimChain = AnimChainT<10>;

#endif
