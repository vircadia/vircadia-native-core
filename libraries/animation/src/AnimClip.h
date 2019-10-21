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

    AnimClip(const QString& id, const QString& url, float startFrame, float endFrame, float timeScale, bool loopFlag, bool mirrorFlag,
             AnimBlendType blendType, const QString& baseURL, float baseFrame);
    virtual ~AnimClip() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

    void setStartFrameVar(const QString& startFrameVar) { _startFrameVar = startFrameVar; }
    void setEndFrameVar(const QString& endFrameVar) { _endFrameVar = endFrameVar; }
    void setTimeScaleVar(const QString& timeScaleVar) { _timeScaleVar = timeScaleVar; }
    void setLoopFlagVar(const QString& loopFlagVar) { _loopFlagVar = loopFlagVar; }
    void setMirrorFlagVar(const QString& mirrorFlagVar) { _mirrorFlagVar = mirrorFlagVar; }
    void setFrameVar(const QString& frameVar) { _frameVar = frameVar; }

    float getStartFrame() const { return _startFrame; }
    void setStartFrame(float startFrame) { _startFrame = startFrame; }
    float getEndFrame() const { return _endFrame; }
    void setEndFrame(float endFrame) { _endFrame = endFrame; }

    void setTimeScale(float timeScale) { _timeScale = timeScale; }
    float getTimeScale() const { return _timeScale; }

    bool getLoopFlag() const { return _loopFlag; }
    void setLoopFlag(bool loopFlag) { _loopFlag = loopFlag; }

    bool getMirrorFlag() const { return _mirrorFlag; }
    void setMirrorFlag(bool mirrorFlag) { _mirrorFlag = mirrorFlag; }

    float getFrame() const { return _frame; }
    void loadURL(const QString& url);

    AnimBlendType getBlendType() const { return _blendType; };

protected:

    virtual void setCurrentFrameInternal(float frame) override;

    void buildMirrorAnim();

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimationPointer _networkAnim;
    AnimationPointer _baseNetworkAnim;

    AnimPoseVec _poses;

    // _anim[frame][joint]
    std::vector<AnimPoseVec> _anim;
    std::vector<AnimPoseVec> _mirrorAnim;

    QString _url;
    float _startFrame;
    float _endFrame;
    float _timeScale;
    bool _loopFlag;
    bool _mirrorFlag;
    float _frame;
    AnimBlendType _blendType;
    QString _baseURL;
    float _baseFrame;

    QString _startFrameVar;
    QString _endFrameVar;
    QString _timeScaleVar;
    QString _loopFlagVar;
    QString _mirrorFlagVar;
    QString _frameVar;

    // no copies
    AnimClip(const AnimClip&) = delete;
    AnimClip& operator=(const AnimClip&) = delete;
};

#endif // hifi_AnimClip_h
