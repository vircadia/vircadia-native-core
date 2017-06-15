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

class LeapMotionPlugin : public InputPlugin {
    Q_OBJECT

public:
    virtual const QString getName() const override { return NAME; }

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

protected:
    static const char* NAME;

    class InputDevice : public controller::InputDevice {
    public:
        friend class LeapMotionPlugin;

        InputDevice() : controller::InputDevice("Leap Motion") {}

        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
    };

    std::shared_ptr<InputDevice> _inputDevice{ std::make_shared<InputDevice>() };
};

#endif // hifi_LeapMotionPlugin_h
