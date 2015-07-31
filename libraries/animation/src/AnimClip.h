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

class AnimClip : public AnimNode {

    void setURL(const std::string& url);
    void setStartFrame(AnimFrame startFrame);
    void setEndFrame(AnimFrame startFrame);
    void setLoopFlag(bool loopFlag);
    void setTimeScale(float timeScale);

public:
    virtual const float getEnd() const;
    virtual const AnimPose& evaluate(float t);
};

#endif // hifi_AnimClip_h
