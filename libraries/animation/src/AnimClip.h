//
//  AnimClip.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimClip_h
#define hifi_AnimClip_h

#include <string>
#include "AnimationCache.h"
#include "AnimNode.h"

class AnimClip : public AnimNode {
public:
    friend class AnimClipTests;

    AnimClip(const std::string& id, const std::string& url, float startFrame, float endFrame, float timeScale, bool loopFlag);
    virtual ~AnimClip() override;

    void setURL(const std::string& url);
    const std::string& getURL() const { return _url; }

    void setStartFrame(float startFrame);
    float getStartFrame() const { return _startFrame; }

    void setEndFrame(float endFrame);
    float getEndFrame() const { return _endFrame; }

    void setTimeScale(float timeScale) { _timeScale = timeScale; }
    float getTimeScale() const { return _timeScale; }

    void setLoopFlag(bool loopFlag);
    bool getLoopFlag() const { return _loopFlag; }

    virtual const std::vector<AnimPose>& evaluate(float dt) override;

protected:
    float accumulateTime(float frame, float dt) const;
    void copyFromNetworkAnim();

    // for AnimDebugDraw rendering
    virtual const std::vector<AnimPose>& getPosesInternal() const override;

    AnimationPointer _networkAnim;
    std::vector<AnimPose> _poses;

    // _anim[frame][joint]
    std::vector<std::vector<AnimPose>> _anim;

    std::string _url;
    float _startFrame;
    float _endFrame;
    float _timeScale;
    bool _loopFlag;

    float _frame;

    // no copies
    AnimClip(const AnimClip&) = delete;
    AnimClip& operator=(const AnimClip&) = delete;
};

#endif // hifi_AnimClip_h
