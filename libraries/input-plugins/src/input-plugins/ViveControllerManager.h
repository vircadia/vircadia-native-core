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
#include "InputDevice.h"
#include "InputPlugin.h"
#include <RenderArgs.h>
#include <render/Scene.h>

class ViveControllerManager : public InputPlugin, public InputDevice {
    Q_OBJECT
public:
    enum JoystickAxisChannel {
        AXIS_Y_POS = 1U << 1,
        AXIS_Y_NEG = 1U << 2,
        AXIS_X_POS = 1U << 3,
        AXIS_X_NEG = 1U << 4,
        BACK_TRIGGER = 1U << 5,
    };

    enum Axis {
        TRACKPAD_AXIS = 0,
        TRIGGER_AXIS,
        AXIS_2,
        AXIS_3,
        AXIS_4,
    };

    enum JointChannel {
        LEFT_HAND = 0,
        RIGHT_HAND,
    };

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
    virtual void registerToUserInputMapper(UserInputMapper& mapper) override;
    virtual void assignDefaultInputMapping(UserInputMapper& mapper) override;
    virtual void update(float deltaTime, bool jointsCaptured) override;
    virtual void focusOutEvent() override;

    void updateRendering(RenderArgs* args, render::ScenePointer scene, render::PendingChanges pendingChanges);

    void setRenderControllers(bool renderControllers) { _renderControllers = renderControllers; }
    
    UserInputMapper::Input makeInput(unsigned int button, int index);
    UserInputMapper::Input makeInput(JoystickAxisChannel axis, int index);
    UserInputMapper::Input makeInput(JointChannel joint);
    
private:
    void renderHand(UserInputMapper::PoseValue pose, gpu::Batch& batch, int index);
    
    void handleButtonEvent(uint64_t buttons, int index);
    void handleAxisEvent(Axis axis, float x, float y, int index);
    void handlePoseEvent(const mat4& mat, int index);
    
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
