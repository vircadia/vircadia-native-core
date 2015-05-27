//
//  DisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
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

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <RegisteredMetaTypes.h>

enum class Eye {
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

class DisplayPlugin : public Plugin {
    Q_OBJECT
public:
    virtual bool isHmd() const { return false; }
    virtual bool isStereo() const { return false; }
    virtual bool isThrottled() const { return false; }

    // Rendering support
    virtual void preRender() {};

    virtual void preDisplay() {
        makeCurrent();
    };

    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
                         GLuint overlayTexture, const glm::uvec2& overlaySize) = 0;

    virtual void finishFrame() {
        swapBuffers();
        doneCurrent();
    };
    
    // Does the rendering surface have current focus?
    virtual bool hasFocus() const = 0;
    // The size of the rendering surface
    virtual QSize getDeviceSize() const = 0;
    // The size of the rendering target (may be larger than the device size due to distortion)
    virtual QSize getRecommendedFramebufferSize() const { return getDeviceSize(); };
    // The size of the window (differs from the framebuffers size in instances like Retina macs)
    virtual glm::ivec2 getCanvasSize() const = 0;

    // The mouse position relative to the window (or proxy window) surface
    virtual glm::ivec2 getTrueMousePosition() const = 0;

    // The mouse position relative to the UI elements
    virtual glm::ivec2 getUiMousePosition() const {
        return trueMouseToUiMouse(getTrueMousePosition()); 
    }

    virtual std::function<QPointF(const QPointF&)> getMouseTranslator() { return [](const QPointF& p) { return p; }; };

    // Convert from screen mouse coordinates to UI mouse coordinates
    virtual glm::ivec2 trueMouseToUiMouse(const glm::ivec2 & position) const { return position; };

    virtual PickRay computePickRay(const glm::vec2 & pos) const {
        // FIXME make pure virtual
        return PickRay();
    };
    virtual bool isMouseOnScreen() const;


    // Stereo specific methods
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const {
        return baseProjection;
    }

    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const {
        return glm::inverse(getEyePose(eye)) * baseModelview;
    }

    // HMD specific methods
    // TODO move these into another class
    virtual glm::mat4 getEyePose(Eye eye) const {
        static const glm::mat4 pose; return pose;
    }

    virtual glm::mat4 getHeadPose() const {
        static const glm::mat4 pose; return pose;
    }

    virtual void abandonCalibration() {}
    virtual void resetSensors() {}
    virtual float devicePixelRatio() { return 1.0;  }
    
signals:
    void recommendedFramebufferSizeChanged(const QSize & size);
    void requestRender();

protected:
    virtual void makeCurrent() {}
    virtual void doneCurrent() {}
    virtual void swapBuffers() {}
};

