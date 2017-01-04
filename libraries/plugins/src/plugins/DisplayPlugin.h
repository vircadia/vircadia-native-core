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
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonObject>

#include <GLMHelpers.h>
#include <RegisteredMetaTypes.h>
#include <shared/Bilateral.h>
#include <gpu/Forward.h>

#include "Plugin.h"

class QImage;

enum Eye {
    Left = (int)bilateral::Side::Left,
    Right = (int)bilateral::Side::Right
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

// Stereo display functionality
// TODO move out of this file don't derive DisplayPlugin from this.  Instead use dynamic casting when 
// displayPlugin->isStereo returns true
class StereoDisplay {
public:
    // Stereo specific methods
    virtual glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const {
        return baseProjection;
    }

    virtual glm::mat4 getCullingProjection(const glm::mat4& baseProjection) const {
        return baseProjection;
    }

    virtual float getIPD() const { return AVERAGE_HUMAN_IPD; }
};

// HMD display functionality
// TODO move out of this file don't derive DisplayPlugin from this.  Instead use dynamic casting when 
// displayPlugin->isHmd returns true
class HmdDisplay : public StereoDisplay {
public:
    // HMD specific methods
    // TODO move these into another class?
    virtual glm::mat4 getEyeToHeadTransform(Eye eye) const {
        static const glm::mat4 transform; return transform;
    }

    // returns a copy of the most recent head pose, computed via updateHeadPose
    virtual glm::mat4 getHeadPose() const {
        return glm::mat4();
    }

    virtual void abandonCalibration() {}

    virtual void resetSensors() {}

    enum Hand {
        LeftHand = 0x01,
        RightHand = 0x02,
    };

    enum class HandLaserMode {
        None, // Render no hand lasers
        Overlay, // Render hand lasers only if they intersect with the UI layer, and stop at the UI layer
    };

    virtual bool setHandLaser(
        uint32_t hands, // Bits from the Hand enum
        HandLaserMode mode, // Mode in which to render
        const vec4& color = vec4(1), // The color of the rendered laser
        const vec3& direction = vec3(0, 0, -1) // The direction in which to render the hand lasers
        ) {
        return false;
    }

    virtual bool setExtraLaser(HandLaserMode mode, const vec4& color, const glm::vec3& sensorSpaceStart, const vec3& sensorSpaceDirection) {
        return false;
    }

    virtual bool suppressKeyboard() { return false;  }
    virtual void unsuppressKeyboard() {};
    virtual bool isKeyboardVisible() { return false; }
};

class DisplayPlugin : public Plugin, public HmdDisplay {
    Q_OBJECT
    using Parent = Plugin;
public:
    enum Event {
        Present = QEvent::User + 1
    };

    virtual int getRequiredThreadCount() const { return 0; }
    virtual bool isHmd() const { return false; }
    virtual int getHmdScreen() const { return -1; }
    /// By default, all HMDs are stereo
    virtual bool isStereo() const { return isHmd(); }
    virtual bool isThrottled() const { return false; }
    virtual float getTargetFrameRate() const { return 0.0f; }
    virtual bool hasAsyncReprojection() const { return false; }

    /// Returns a boolean value indicating whether the display is currently visible 
    /// to the user.  For monitor displays, false might indicate that a screensaver,
    /// or power-save mode is active.  For HMDs it may reflect a sensor indicating
    /// whether the HMD is being worn
    virtual bool isDisplayVisible() const { return false; }

    virtual QString getPreferredAudioInDevice() const { return QString(); }
    virtual QString getPreferredAudioOutDevice() const { return QString(); }

    // Rendering support
    virtual void setContext(const gpu::ContextPointer& context) final { _gpuContext = context; }
    virtual void submitFrame(const gpu::FramePointer& newFrame) = 0;

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

    // The recommended bounds for primary overlay placement
    virtual QRect getRecommendedOverlayRect() const {
        const int DESKTOP_SCREEN_PADDING = 50;
        auto recommendedSize = getRecommendedUiSize() - glm::uvec2(DESKTOP_SCREEN_PADDING);
        return QRect(0, 0, recommendedSize.x, recommendedSize.y);
    }

    // Fetch the most recently displayed image as a QImage
    virtual QImage getScreenshot(float aspectRatio = 0.0f) const = 0;

    // will query the underlying hmd api to compute the most recent head pose
    virtual bool beginFrameRender(uint32_t frameIndex) { return true; }

    virtual float devicePixelRatio() { return 1.0f; }
    // Rate at which we render frames
    virtual float renderRate() const { return -1.0f; }
    // Rate at which we present to the display device
    virtual float presentRate() const { return -1.0f; }
    // Rate at which old frames are presented to the device display
    virtual float stutterRate() const { return -1.0f; }
    // Rate at which new frames are being presented to the display device
    virtual float newFramePresentRate() const { return -1.0f; }
    // Rate at which rendered frames are being skipped
    virtual float droppedFrameRate() const { return -1.0f; }
    
    // Hardware specific stats
    virtual QJsonObject getHardwareStats() const { return QJsonObject(); }

    uint32_t presentCount() const { return _presentedFrameIndex; }
    // Time since last call to incrementPresentCount (only valid if DEBUG_PAINT_DELAY is defined)
    int64_t getPaintDelayUsecs() const;

    virtual void cycleDebugOutput() {}

    static const QString& MENU_PATH();


signals:
    void recommendedFramebufferSizeChanged(const QSize& size);
    void resetSensorsRequested();

protected:
    void incrementPresentCount();

    gpu::ContextPointer _gpuContext;

private:
    std::atomic<uint32_t> _presentedFrameIndex;
    mutable std::mutex _paintDelayMutex;
    QElapsedTimer _paintDelayTimer;
};

