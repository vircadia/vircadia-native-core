//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <functional>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QSize>
#include <QtCore/QPoint>
class QImage;

#include <GLMHelpers.h>
#include <RegisteredMetaTypes.h>

#include "Plugin.h"

enum Eye {
    Left,
    Right
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

#define AVERAGE_HUMAN_IPD 0.064f

namespace gpu {
    class Texture;
    using TexturePointer = std::shared_ptr<Texture>;
}

class DisplayPlugin : public Plugin {
    Q_OBJECT
    using Parent = Plugin;
public:
    void activate() override;
    void deactivate() override;
    virtual bool isHmd() const { return false; }
    virtual int getHmdScreen() const { return -1; }
    /// By default, all HMDs are stereo
    virtual bool isStereo() const { return isHmd(); }
    virtual bool isThrottled() const { return false; }
    virtual float getTargetFrameRate() { return 0.0f; }

    /// Returns a boolean value indicating whether the display is currently visible 
    /// to the user.  For monitor displays, false might indicate that a screensaver,
    /// or power-save mode is active.  For HMDs it may reflect a sensor indicating
    /// whether the HMD is being worn
    virtual bool isDisplayVisible() const { return false; }

    // Rendering support

    /**
     *  Sends the scene texture to the display plugin.
     */
    virtual void submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) = 0;

    /**
    *  Sends the scene texture to the display plugin.
    */
    virtual void submitOverlayTexture(const gpu::TexturePointer& overlayTexture) = 0;

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
    virtual glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const {
        return baseProjection;
    }

    virtual glm::mat4 getCullingProjection(const glm::mat4& baseProjection) const {
        return baseProjection;
    }


    // Fetch the most recently displayed image as a QImage
    virtual QImage getScreenshot() const = 0;

    // HMD specific methods
    // TODO move these into another class?
    virtual glm::mat4 getEyeToHeadTransform(Eye eye) const {
        static const glm::mat4 transform; return transform;
    }

    // will query the underlying hmd api to compute the most recent head pose
    virtual void updateHeadPose(uint32_t frameIndex) {}

    // returns a copy of the most recent head pose, computed via updateHeadPose
    virtual glm::mat4 getHeadPose() const {
        return glm::mat4();
    }

    // Needed for timewarp style features
    virtual void setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) {
        // NOOP
    }

    virtual float getIPD() const { return AVERAGE_HUMAN_IPD; }

    virtual void abandonCalibration() {}
    virtual void resetSensors() {}
    virtual float devicePixelRatio() { return 1.0f; }
    virtual float presentRate() { return -1.0f; }
    uint32_t presentCount() const { return _presentedFrameIndex; }

    static const QString& MENU_PATH();

signals:
    void recommendedFramebufferSizeChanged(const QSize & size);

protected:
    void incrementPresentCount() { ++_presentedFrameIndex; }

private:
    std::atomic<uint32_t> _presentedFrameIndex;
};

