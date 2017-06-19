//
//  LeapMotionPlugin.h
//
//  Created by David Rowe on 15 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LeapMotionPlugin_h
#define hifi_LeapMotionPlugin_h

#include <controllers/InputDevice.h>
#include <plugins/InputPlugin.h>

// LeapMotion SDK
#include <Leap.h>

class LeapMotionPlugin : public InputPlugin {
    Q_OBJECT

public:
    // InputPlugin methods
    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    bool isHandController() const override { return true; }

    // Plugin methods
    virtual const QString getName() const override { return NAME; }
    const QString getID() const override { return LEAPMOTION_ID_STRING; }

    virtual void init() override;

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

protected:
    static const char* NAME;
    static const char* LEAPMOTION_ID_STRING;
    const float DEFAULT_DESKTOP_HEIGHT_OFFSET = 0.2f;

    bool _enabled { false };
    QString _sensorLocation;
    float _desktopHeightOffset { DEFAULT_DESKTOP_HEIGHT_OFFSET };

    struct LeapMotionJoint {
        glm::vec3 position;
        glm::quat orientation;
    };

    std::vector<LeapMotionJoint> _joints;
    std::vector<LeapMotionJoint> _prevJoints;

    class InputDevice : public controller::InputDevice {
    public:
        friend class LeapMotionPlugin;

        InputDevice() : controller::InputDevice("LeapMotion") {}

        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override {};
        virtual void focusOutEvent() override {};

        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
            const std::vector<LeapMotionPlugin::LeapMotionJoint>& joints, 
            const std::vector<LeapMotionPlugin::LeapMotionJoint>& prevJoints);

        void clearState();

        void setDektopHeightOffset(float desktopHeightOffset) { _desktopHeightOffset = desktopHeightOffset; };
        void setIsLeapOnHMD(bool isLeapOnHMD) { _isLeapOnHMD = isLeapOnHMD; };

    private:
        float _desktopHeightOffset { 0.0f };
        bool _isLeapOnHMD { false };
    };

    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

private:
    void applySensorLocation();
    void applyDesktopHeightOffset();

    void processFrame(const Leap::Frame& frame);

    Leap::Controller _controller;

    bool _hasLeapMotionBeenConnected { false };
    int64_t _lastFrameID { -1 };
};

#endif // hifi_LeapMotionPlugin_h
