//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "plugins/Plugin.h"
#include <QSize>
#include <QPoint>
#include <functional>

#include "gpu/GPUConfig.h"
#include "GLMHelpers.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <RegisteredMetaTypes.h>

enum Eye {
    Left,
    Right,
    Mono
};

/*
 * Helper method to iterate over each eye
 */
template <typename F>
void for_each_eye(F f) {
    f(Left);
    f(Right);
}

/*
 * Helper method to iterate over each eye, with an additional lambda to take action between the eyes
 */
template <typename F, typename FF>
void for_each_eye(F f, FF ff) {
    f(Eye::Left);
    ff();
    f(Eye::Right);
}

class QWindow;

class DisplayPlugin : public Plugin {
    Q_OBJECT
public:
    virtual bool isHmd() const { return false; }
    virtual int getHmdScreen() const { return -1; }
    /// By default, all HMDs are stereo
    virtual bool isStereo() const { return isHmd(); }
    virtual bool isThrottled() const { return false; }

    // Rendering support

    // Stop requesting renders, but don't do full deactivation
    // needed to work around the issues caused by Oculus 
    // processing messages in the middle of submitFrame
    virtual void stop() = 0;

    /**
     *  Called by the application before the frame rendering.  Can be used for
     *  render timing related calls (for instance, the Oculus begin frame timing
     *  call)
     */
    virtual void preRender() = 0;
    /**
     *  Called by the application immediately before calling the display function.
     *  For OpenGL based plugins, this is the best place to put activate the output
     *  OpenGL context
     */
    virtual void preDisplay() = 0;

    /**
     *  Sends the scene texture to the display plugin.
     */
    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize) = 0;

    /**
     *  Called by the application immeidately after display.  For OpenGL based
     *  displays, this is the best place to put the buffer swap
     */
    virtual void finishFrame() = 0;

    // Does the rendering surface have current focus?
    virtual bool hasFocus() const = 0;

    // The size of the rendering target (may be larger than the device size due to distortion)
    virtual glm::uvec2 getRecommendedRenderSize() const = 0;

    // The size of the UI
    virtual glm::uvec2 getRecommendedUiSize() const {
        return getRecommendedRenderSize();
    }

    // By default the aspect ratio is just the render size
    virtual float getRecommendedAspectRatio() const {
        return aspect(getRecommendedRenderSize());
    }

    // Stereo specific methods
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const {
        return baseProjection;
    }

    virtual glm::mat4 getView(Eye eye, const glm::mat4& baseView) const {
        return glm::inverse(getEyePose(eye)) * baseView;
    }

    // HMD specific methods
    // TODO move these into another class?
    virtual glm::mat4 getEyePose(Eye eye) const {
        static const glm::mat4 pose; return pose;
    }

    virtual glm::mat4 getHeadPose() const {
        static const glm::mat4 pose; return pose;
    }

    virtual float getIPD() const { return 0.0f; }

    virtual void abandonCalibration() {}
    virtual void resetSensors() {}
    virtual float devicePixelRatio() { return 1.0;  }


    static const QString& MENU_PATH();
signals:
    void recommendedFramebufferSizeChanged(const QSize & size);
    void requestRender();
};

