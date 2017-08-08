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
#include <vector>
#include <map>
#include <utility>

#include <GLMHelpers.h>
#include <model/Geometry.h>
#include <gpu/Texture.h>
#include <controllers/InputDevice.h>
#include <plugins/InputPlugin.h>
#include "OpenVrHelpers.h"

using PuckPosePair = std::pair<uint32_t, controller::Pose>;

namespace vr {
    class IVRSystem;
}

class ViveControllerManager : public InputPlugin {
    Q_OBJECT
public:
    // Plugin functions
    bool isSupported() const override;
    const QString getName() const override { return NAME; }

    bool isHandController() const override { return true; }
    bool configurable() override { return true; }

    QString configurationLayout() override;
    void setConfigurationSettings(const QJsonObject configurationSettings) override;
    QJsonObject configurationSettings() override;
    void calibrate() override;
    bool uncalibrate() override;
    bool isHeadController() const override { return true; }
    bool isHeadControllerMounted() const;

    bool activate() override;
    void deactivate() override;

    void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    void setRenderControllers(bool renderControllers) { _renderControllers = renderControllers; }

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

private:
    class InputDevice : public controller::InputDevice {
    public:
        InputDevice(vr::IVRSystem*& system);
        bool isHeadControllerMounted() const { return _overrideHead; }
    private:
        // Device functions
        controller::Input::NamedVector getAvailableInputs() const override;
        QString getDefaultMappingConfig() const override;
        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        void focusOutEvent() override;
        bool triggerHapticPulse(float strength, float duration, controller::Hand hand) override;
        void hapticsHelper(float deltaTime, bool leftHand);
        void calibrateOrUncalibrate(const controller::InputCalibrationData& inputCalibration);
        void calibrate(const controller::InputCalibrationData& inputCalibration);
        void uncalibrate();
        void sendUserActivityData(QString activity);
        void configureCalibrationSettings(const QJsonObject configurationSettings);
        QJsonObject configurationSettings();
        controller::Pose addOffsetToPuckPose(int joint) const;
        glm::mat4 calculateDefaultToReferenceForHeadPuck(const controller::InputCalibrationData& inputCalibration);
        glm::mat4 calculateDefaultToReferenceForHmd(const controller::InputCalibrationData& inputCalibration);
        void updateCalibratedLimbs();
        bool checkForCalibrationEvent();
        void handleHandController(float deltaTime, uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData, bool isLeftHand);
        void handleHmd(uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData);
        void handleTrackedObject(uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData);
        void handleButtonEvent(float deltaTime, uint32_t button, bool pressed, bool touched, bool isLeftHand);
        void handleAxisEvent(float deltaTime, uint32_t axis, float x, float y, bool isLeftHand);
        void handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, const mat4& mat,
                             const vec3& linearVelocity, const vec3& angularVelocity, bool isLeftHand);
        void handleHeadPoseEvent(const controller::InputCalibrationData& inputCalibrationData, const mat4& mat, const vec3& linearVelocity,
                                 const vec3& angularVelocity);
        void partitionTouchpad(int sButton, int xAxis, int yAxis, int centerPsuedoButton, int xPseudoButton, int yPseudoButton);
        void printDeviceTrackingResultChange(uint32_t deviceIndex);
        void setConfigFromString(const QString& value);
        bool configureHead(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration);
        bool configureHands(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration);
        bool configureBody(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration);
        void calibrateLeftHand(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& handPair);
        void calibrateRightHand(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& handPair);
        void calibrateFeet(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration);
        void calibrateFoot(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& footPair, bool isLeftFoot);
        void calibrateHips(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration);
        void calibrateChest(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration);
        void calibrateShoulders(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration,
                                int firstShoulderIndex, int secondShoulderIndex);
        void calibrateHead(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration);
        void calibrateFromHandController(const controller::InputCalibrationData& inputCalibrationData);
        void calibrateFromUI(const controller::InputCalibrationData& inputCalibrationData);
        void emitCalibrationStatus();
        void calibrateNextFrame();


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
        enum class Config {
            None,
            Feet,
            FeetAndHips,
            FeetHipsAndChest,
            FeetHipsAndShoulders,
            FeetHipsChestAndShoulders
        };

        enum class HeadConfig {
            HMD,
            Puck
        };

        enum class HandConfig {
            HandController,
            Pucks
        };

        Config _config { Config::None };
        Config _preferedConfig { Config::None };
        HeadConfig _headConfig { HeadConfig::HMD };
        HandConfig _handConfig { HandConfig::HandController };
        FilteredStick _filteredLeftStick;
        FilteredStick _filteredRightStick;

        std::vector<PuckPosePair> _validTrackedObjects;
        std::map<uint32_t, glm::mat4> _pucksPostOffset;
        std::map<uint32_t, glm::mat4> _pucksPreOffset;
        std::map<int, uint32_t> _jointToPuckMap;
        std::map<Config, QString> _configStringMap;
        PoseData _lastSimPoseData;
        // perform an action when the InputDevice mutex is acquired.
        using Locker = std::unique_lock<std::recursive_mutex>;
        template <typename F>
        void withLock(F&& f) { Locker locker(_lock); f(); }

        int _trackedControllers { 0 };
        vr::IVRSystem*& _system;
        quint64 _timeTilCalibration { 0 };
        float _leftHapticStrength { 0.0f };
        float _leftHapticDuration { 0.0f };
        float _rightHapticStrength { 0.0f };
        float _rightHapticDuration { 0.0f };
        float _headPuckYOffset { -0.05f };
        float _headPuckZOffset { -0.05f };
        float _handPuckYOffset { 0.0f };
        float _handPuckZOffset { 0.0f };
        float _armCircumference { 0.33f };
        float _shoulderWidth { 0.48f };
        bool _triggersPressedHandled { false };
        bool _calibrated { false };
        bool _timeTilCalibrationSet { false };
        bool _calibrate { false };
        bool _overrideHead { false };
        bool _overrideHands { false };
        mutable std::recursive_mutex _lock;

        QString configToString(Config config);
        friend class ViveControllerManager;
    };

    void renderHand(const controller::Pose& pose, gpu::Batch& batch, int sign);
    bool isDesktopMode();
    bool _registeredWithInputMapper { false };
    bool _modelLoaded { false };
    bool _resetMatCalculated { false };
    bool _desktopMode { false };
    glm::mat4 _resetMat { glm::mat4() };
    model::Geometry _modelGeometry;
    gpu::TexturePointer _texture;

    int _leftHandRenderID { 0 };
    int _rightHandRenderID { 0 };

    bool _renderControllers { false };
    vr::IVRSystem* _system { nullptr };
    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>(_system) };

    static const char* NAME;
};

#endif // hifi__ViveControllerManager
