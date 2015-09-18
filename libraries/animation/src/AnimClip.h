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

    AnimClip(const QString& id, const QString& url, float startFrame, float endFrame, float timeScale, bool loopFlag);
    virtual ~AnimClip() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) override;

    void setStartFrameVar(const QString& startFrameVar) { _startFrameVar = startFrameVar; }
    void setEndFrameVar(const QString& endFrameVar) { _endFrameVar = endFrameVar; }
    void setTimeScaleVar(const QString& timeScaleVar) { _timeScaleVar = timeScaleVar; }
    void setLoopFlagVar(const QString& loopFlagVar) { _loopFlagVar = loopFlagVar; }
    void setFrameVar(const QString& frameVar) { _frameVar = frameVar; }

protected:
    void loadURL(const QString& url);

    virtual void setCurrentFrameInternal(float frame) override;

    float accumulateTime(float frame, float dt, Triggers& triggersOut) const;
    void copyFromNetworkAnim();

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimationPointer _networkAnim;
    AnimPoseVec _poses;

    // _anim[frame][joint]
    std::vector<AnimPoseVec> _anim;

    QString _url;
    float _startFrame;
    float _endFrame;
    float _timeScale;
    bool _loopFlag;
    float _frame;

    QString _startFrameVar;
    QString _endFrameVar;
    QString _timeScaleVar;
    QString _loopFlagVar;
    QString _frameVar;

    // no copies
    AnimClip(const AnimClip&) = delete;
    AnimClip& operator=(const AnimClip&) = delete;
};

#endif // hifi_AnimClip_h
