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

#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <RegisteredMetaTypes.h>

class DisplayPlugin : public Plugin {
    Q_OBJECT
public:
    virtual bool isHmd() const { return false; }
    virtual bool isStereo() const { return false; }
    virtual bool isThrottled() const { return false; }

    // Rendering support
    virtual void preDisplay() {};
    virtual void display(int finalSceneTexture) {};
    virtual void postDisplay() {};

    // Pointer support

    // Does the rendering surface have current focus?
    virtual bool hasFocus() const = 0;
    // The size of the rendering surface
    virtual QSize getRecommendedFramebufferSize() const = 0;
    // The size of the window (differs from the framebuffers size in instances like Retina macs)
    virtual glm::ivec2 getCanvasSize() const = 0;

    // The mouse position relative to the window (or proxy window) surface
    virtual glm::ivec2 getTrueMousePosition() const = 0;

    // The mouse position relative to the UI elements
    virtual glm::ivec2 getUiMousePosition() const {
        return trueMouseToUiMouse(getTrueMousePosition()); 
    }

    virtual std::function<QPointF(QPointF)> getMouseTranslator() { return [](const QPointF& p) { return p; }; };

    // Convert from screen mouse coordinates to UI mouse coordinates
    virtual glm::ivec2 trueMouseToUiMouse(const glm::ivec2 & position) const { return position; };

    virtual PickRay computePickRay(const glm::vec2 & pos) const = 0;
    virtual bool isMouseOnScreen() const;
    virtual void overrideOffAxisFrustum(
        float& left, float& right, float& bottom, float& top,
        float& nearVal, float& farVal,
        glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const { }

    // HMD specific methods
    // TODO move these into another class
    virtual glm::mat4 headPose() const {
        static const glm::mat4 pose; return pose;
    }
    virtual glm::quat headOrientation() const {
        static const glm::quat orientation; return orientation;
    }
    virtual glm::vec3 headTranslation() const {
        static const glm::vec3 tranlsation; return tranlsation;
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
