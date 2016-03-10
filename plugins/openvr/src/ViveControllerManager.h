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
        InputDevice(vr::IVRSystem*& system) : controller::InputDevice("Vive"), _system(system) {}
    private:
        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) override;
        virtual void focusOutEvent() override;

        void handleHandController(float deltaTime, uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData, bool isLeftHand);
        void handleButtonEvent(float deltaTime, uint32_t button, bool pressed, bool isLeftHand);
        void handleAxisEvent(float deltaTime, uint32_t axis, float x, float y, bool isLeftHand);
        void handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, const mat4& mat,
                             const vec3& linearVelocity, const vec3& angularVelocity, bool isLeftHand);

        class FilteredStick {
        public:
            glm::vec2 process(float deltaTime, const glm::vec2& stick) {
                // Use a timer to prevent the stick going to back to zero.
                // This to work around the noisy touch pad that will flash back to zero breifly
                const float ZERO_HYSTERESIS_PERIOD = 0.2f;  // 200 ms
                if (glm::length(stick) == 0.0f) {
                    if (_timer <= 0.0f) {
                        return glm::vec2(0.0f, 0.0f);
                    } else {
                        _timer -= deltaTime;
                        return _stick;
                    }
                } else {
                    _timer = ZERO_HYSTERESIS_PERIOD;
                    _stick = stick;
                    return stick;
                }
            }
        protected:
            float _timer { 0.0f };
            glm::vec2 _stick { 0.0f, 0.0f };
        };

        FilteredStick _filteredLeftStick;
        FilteredStick _filteredRightStick;

        int _trackedControllers { 0 };
        vr::IVRSystem*& _system;
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
    vr::IVRSystem* _system { nullptr };
    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>(_system) };

    static const QString NAME;


};

#endif // hifi__ViveControllerManager
