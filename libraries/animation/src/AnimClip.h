//
//  AnimClip.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimClip_h
#define hifi_AnimClip_h

#include <string>
#include "AnimationCache.h"
#include "AnimNode.h"

// Playback a single animation timeline.
// url determines the location of the fbx file to use within this clip.
// startFrame and endFrame are in frames 1/30th of a second.
// timescale can be used to speed-up or slow-down the animation.
// loop flag, when true, will loop the animation as it reaches the end frame.

class AnimClip : public AnimNode {
public:
    friend class AnimTests;

    AnimClip(const std::string& id, const std::string& url, float startFrame, float endFrame, float timeScale, bool loopFlag);
    virtual ~AnimClip() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) override;

    void setStartFrameVar(const std::string& startFrameVar) { _startFrameVar = startFrameVar; }
    void setEndFrameVar(const std::string& endFrameVar) { _endFrameVar = endFrameVar; }
    void setTimeScaleVar(const std::string& timeScaleVar) { _timeScaleVar = timeScaleVar; }
    void setLoopFlagVar(const std::string& loopFlagVar) { _loopFlagVar = loopFlagVar; }
    void setFrameVar(const std::string& frameVar) { _frameVar = frameVar; }

protected:
    void loadURL(const std::string& url);

    virtual void setCurrentFrameInternal(float frame) override;

    float accumulateTime(float frame, float dt, Triggers& triggersOut) const;
    void copyFromNetworkAnim();

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimationPointer _networkAnim;
    AnimPoseVec _poses;

    // _anim[frame][joint]
    std::vector<AnimPoseVec> _anim;

    std::string _url;
    float _startFrame;
    float _endFrame;
    float _timeScale;
    bool _loopFlag;
    float _frame;

    std::string _startFrameVar;
    std::string _endFrameVar;
    std::string _timeScaleVar;
    std::string _loopFlagVar;
    std::string _frameVar;

    // no copies
    AnimClip(const AnimClip&) = delete;
    AnimClip& operator=(const AnimClip&) = delete;
};

#endif // hifi_AnimClip_h
