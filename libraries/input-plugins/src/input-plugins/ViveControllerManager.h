//
//  ViveControllerManager.h
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__ViveControllerManager
#define hifi__ViveControllerManager

#include <QObject>
#include <unordered_set>

#include <GLMHelpers.h>

#include <model/Geometry.h>
#include <gpu/Texture.h>
#include <controllers/InputDevice.h>
#include "InputPlugin.h"
#include <RenderArgs.h>
#include <render/Scene.h>

class ViveControllerManager : public InputPlugin, public controller::InputDevice {
    Q_OBJECT
public:
    ViveControllerManager();

    // Plugin functions
    virtual bool isSupported() const override;
    virtual bool isJointController() const override { return true; }
    const QString& getName() const override { return NAME; }

    virtual void activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, bool jointsCaptured) override { update(deltaTime, jointsCaptured); }

    // Device functions
    virtual void buildDeviceProxy(controller::DeviceProxy::Pointer proxy) override;
    virtual QString getDefaultMappingConfig() override;
    virtual void update(float deltaTime, bool jointsCaptured) override;
    virtual void focusOutEvent() override;

    void updateRendering(RenderArgs* args, render::ScenePointer scene, render::PendingChanges pendingChanges);

    void setRenderControllers(bool renderControllers) { _renderControllers = renderControllers; }
    
private:
    void renderHand(const controller::Pose& pose, gpu::Batch& batch, int sign);
    
    void handleButtonEvent(uint32_t button, bool pressed, bool left);
    void handleAxisEvent(uint32_t axis, float x, float y, bool left);
    void handlePoseEvent(const mat4& mat, bool left);
    
    int _trackedControllers;

    bool _modelLoaded;
    model::Geometry _modelGeometry;
    gpu::TexturePointer _texture;

    int _leftHandRenderID;
    int _rightHandRenderID;

    bool _renderControllers;

    static const QString NAME;
};

#endif // hifi__ViveControllerManager
