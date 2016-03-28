//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "../OpenGLDisplayPlugin.h"
class QScreen;

class StereoDisplayPlugin : public OpenGLDisplayPlugin {
    Q_OBJECT
    using Parent = OpenGLDisplayPlugin;
public:
    virtual bool isStereo() const override final { return true; }
    virtual bool isSupported() const override final;

    virtual float getRecommendedAspectRatio() const override;
    virtual glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const override;

    // NOTE, because Stereo displays don't include head tracking, and therefore 
    // can't include roll or pitch, the eye separation is embedded into the projection
    // matrix.  However, this eliminates the possibility of easily mainpulating
    // the IPD at the Application level, the way we now allow with HMDs.
    // If that becomes an issue then we'll need to break up the functionality similar
    // to the HMD plugins.  
    // virtual glm::mat4 getEyeToHeadTransform(Eye eye) const override;

protected:
    virtual bool internalActivate() override;
    virtual void internalDeactivate() override;
    void updateScreen();

    float _ipd{ 0.064f };
    QScreen* _screen;
};
