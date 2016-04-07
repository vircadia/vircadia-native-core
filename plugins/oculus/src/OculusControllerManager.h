//
//  Created by Bradley Austin Davis on 2016/03/04
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__OculusControllerManager
#define hifi__OculusControllerManager

#include <QObject>
#include <unordered_set>

#include <GLMHelpers.h>

#include <controllers/InputDevice.h>
#include <plugins/InputPlugin.h>

#include <OVR_CAPI.h>

class OculusControllerManager : public InputPlugin {
    Q_OBJECT
public:
    // Plugin functions
    bool isSupported() const override;
    bool isJointController() const override { return true; }
    const QString& getName() const override { return NAME; }

    bool activate() override;
    void deactivate() override;

    void pluginFocusOutEvent() override;
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) override;

private:
    class OculusInputDevice : public controller::InputDevice {
    public:
        OculusInputDevice(OculusControllerManager& parent, const QString& name) : controller::InputDevice(name), _parent(parent) {}

        OculusControllerManager& _parent;
        friend class OculusControllerManager;
    };

    class RemoteDevice : public OculusInputDevice {
    public:
        using Pointer = std::shared_ptr<RemoteDevice>;
        RemoteDevice(OculusControllerManager& parent) : OculusInputDevice(parent, "OculusRemote") {}

        controller::Input::NamedVector getAvailableInputs() const override;
        QString getDefaultMappingConfig() const override;
        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) override;
        void focusOutEvent() override;

        friend class OculusControllerManager;
    };

    class TouchDevice : public OculusInputDevice {
    public:
        using Pointer = std::shared_ptr<TouchDevice>;
        TouchDevice(OculusControllerManager& parent) : OculusInputDevice(parent, "OculusTouch") {}

        controller::Input::NamedVector getAvailableInputs() const override;
        QString getDefaultMappingConfig() const override;
        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) override;
        void focusOutEvent() override;

    private:
        void handlePose(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, ovrHandType hand, const ovrPoseStatef& handPose);
        int _trackedControllers { 0 };
        friend class OculusControllerManager;
    };

    ovrSession _session { nullptr };
    ovrInputState _inputState {};
    RemoteDevice::Pointer _remote;
    TouchDevice::Pointer _touch;
    static const QString NAME;
};

#endif // hifi__OculusControllerManager
