//
//  RenderPlugin.h
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

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <RegisteredMetaTypes.h>

class RenderPlugin : public Plugin {
    Q_OBJECT
public:
    virtual bool isHmd() const { return false; }
    virtual bool isStereo() const { return false; }
    virtual bool isThrottled() const { return false; }

    // Rendering support
    virtual void preRender() {};
    virtual void render(int finalSceneTexture) {};
    virtual void postRender() {};

    // Pointer support
    virtual bool hasFocus() const = 0;
    virtual QSize getRecommendedFramebufferSize() const = 0;
    virtual glm::ivec2 getCanvasSize() const = 0;
    virtual glm::ivec2 getUiMousePosition() const = 0;
    virtual glm::ivec2 getTrueMousePosition() const = 0;
    virtual PickRay computePickRay(const glm::vec2 & pos) const = 0;
    virtual bool isMouseOnScreen() const;

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
    
signals:
    void recommendedFramebufferSizeChanged(const QSize & size);
    void requestRender();

protected:
    virtual void makeCurrent() {}
    virtual void doneCurrent() {}
    virtual void swapBuffers() {}
};
