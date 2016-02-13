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
#include <plugins/InputPlugin.h>
#include <RenderArgs.h>
#include <render/Scene.h>

namespace vr {
    class IVRSystem;
}

class ViveControllerManager : public InputPlugin {
    Q_OBJECT
public:

    // Plugin functions
    virtual bool isSupported() const override;
    virtual bool isJointController() const override { return true; }
    const QString& getName() const override { return NAME; }

    virtual void activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) override;

    void updateRendering(RenderArgs* args, render::ScenePointer scene, render::PendingChanges pendingChanges);

    void setRenderControllers(bool renderControllers) { _renderControllers = renderControllers; }
    
private:
    class InputDevice : public controller::InputDevice {
    public:
        InputDevice(vr::IVRSystem*& hmd) : controller::InputDevice("Vive"), _hmd(hmd) {}
    private:
        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) override;
        virtual void focusOutEvent() override;

        void handleButtonEvent(uint32_t button, bool pressed, bool left);
        void handleAxisEvent(uint32_t axis, float x, float y, bool left);
        void handlePoseEvent(const controller::InputCalibrationData& inputCalibrationData, const mat4& mat, bool left);

        int _trackedControllers { 0 };
        vr::IVRSystem*& _hmd;
        friend class ViveControllerManager;
    };

    void renderHand(const controller::Pose& pose, gpu::Batch& batch, int sign);
    
    

    bool _registeredWithInputMapper { false };
    bool _modelLoaded { false };
    model::Geometry _modelGeometry;
    gpu::TexturePointer _texture;

    int _leftHandRenderID { 0 };
    int _rightHandRenderID { 0 };

    bool _renderControllers { false };
    vr::IVRSystem* _hmd { nullptr };
    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>(_hmd) };

    static const QString NAME;


};

#endif // hifi__ViveControllerManager
