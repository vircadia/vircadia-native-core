//
//  Created by Bradley Austin Davis on 2016/03/04
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__OculusMobileControllerManager
#define hifi__OculusMobileControllerManager

#include <QObject>
#include <unordered_set>
#include <map>

#include <GLMHelpers.h>

#include <controllers/InputDevice.h>
#include <plugins/InputPlugin.h>

class OculusMobileControllerManager : public InputPlugin {
Q_OBJECT
public:
    // Plugin functions
    bool isSupported() const override;
    const QString getName() const override { return NAME; }
    bool isHandController() const override;
    bool isHeadController() const override { return true; }
    QStringList getSubdeviceNames() override;

    bool activate() override;
    void deactivate() override;

    void pluginFocusOutEvent() override;
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

private:
    static const char* NAME;

    void checkForConnectedDevices();
};

#endif // hifi__OculusMobileControllerManager
