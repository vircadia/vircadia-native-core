//
//  AnimDebugDraw.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimDebugDraw_h
#define hifi_AnimDebugDraw_h

#include "render/Scene.h"
#include "gpu/Pipeline.h"
#include "AnimNode.h"
#include "AnimSkeleton.h"

class AnimDebugDrawData;
typedef render::Payload<AnimDebugDrawData> AnimDebugDrawPayload;

class AnimDebugDraw {
public:
    static AnimDebugDraw& getInstance();

    AnimDebugDraw();
    ~AnimDebugDraw();

    void addSkeleton(std::string key, AnimSkeleton::Pointer skeleton, const Transform& worldTransform);
    void removeSkeleton(std::string key);

    void update();

protected:
    std::shared_ptr<AnimDebugDrawData> _animDebugDrawData;
    std::shared_ptr<AnimDebugDrawPayload> _animDebugDrawPayload;
    render::ItemID _itemID;

    static gpu::PipelinePointer _pipeline;

    typedef std::pair<AnimSkeleton::Pointer, Transform> SkeletonInfo;

    std::unordered_map<std::string, SkeletonInfo> _skeletons;
};

#endif // hifi_AnimDebugDraw
