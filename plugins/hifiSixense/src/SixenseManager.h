//
//  SixenseManager.h
//  input-plugins/src/input-plugins
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SixenseManager_h
#define hifi_SixenseManager_h

#include <SimpleMovingAverage.h>

#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>

#include <plugins/InputPlugin.h>

struct _sixenseControllerData;
using SixenseControllerData = _sixenseControllerData;

// Handles interaction with the Sixense SDK (e.g., Razer Hydra).
class SixenseManager : public InputPlugin {
    Q_OBJECT
public:
    // Plugin functions
    virtual bool isSupported() const override;
    virtual const QString getName() const override { return NAME; }
    virtual const QString getID() const override { return HYDRA_ID_STRING; }

    // Sixense always seems to initialize even if the hydras are not present. Is there
    // a way we can properly detect whether the hydras are present?
    bool isHandController() const override { return false; }

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

public slots:
    void setSixenseFilter(bool filter);

private:
    static const int MAX_NUM_AVERAGING_SAMPLES = 50; // At ~100 updates per seconds this means averaging over ~.5s
    static const int CALIBRATION_STATE_IDLE = 0;
    static const int CALIBRATION_STATE_IN_PROGRESS = 1;
    static const int CALIBRATION_STATE_COMPLETE = 2;
    static const glm::vec3 DEFAULT_AVATAR_POSITION;
    static const float CONTROLLER_THRESHOLD;

    class InputDevice : public controller::InputDevice {
    public:
        InputDevice() : controller::InputDevice("Hydra") {}
        void setDebugDrawRaw(bool flag);
        void setDebugDrawCalibrated(bool flag);
    private:
        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        virtual void focusOutEvent() override;

        void handleButtonEvent(unsigned int buttons, bool left);
        void handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, const glm::vec3& position, const glm::quat& rotation, bool left);
        void updateCalibration(SixenseControllerData* controllers);

        friend class SixenseManager;

        int _calibrationState { CALIBRATION_STATE_IDLE };
        // these are calibration results
        glm::vec3 _avatarPosition { DEFAULT_AVATAR_POSITION }; // in hydra-frame
        glm::quat _avatarRotation; // in hydra-frame

        float _lastDistance;
        bool _requestReset { false };
        bool _debugDrawRaw { false };
        bool _debugDrawCalibrated { false };
        // these are measured values used to compute the calibration results
        quint64 _lockExpiry;
        glm::vec3 _averageLeft;
        glm::vec3 _averageRight;
        glm::vec3 _reachLeft;
        glm::vec3 _reachRight;
    };

    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

    static const char* NAME;
    static const char* HYDRA_ID_STRING;

    static bool _sixenseLoaded;
};

#endif // hifi_SixenseManager_h

