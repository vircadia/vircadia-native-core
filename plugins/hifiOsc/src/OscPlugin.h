//
//  OscPlugin.h
//
//  Created by Anthony Thibault on 2019/8/24
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OscPlugin_h
#define hifi_OscPlugin_h

#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>
#include <plugins/InputPlugin.h>

#include "lo/lo.h"

// OSC (Open Sound Control) input plugin.
class OscPlugin : public InputPlugin {
    Q_OBJECT
public:

    // Plugin functions
    virtual void init() override;
    virtual bool isSupported() const override;
    virtual const QString getName() const override { return NAME; }
    const QString getID() const override { return OSC_ID_STRING; }

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

    bool startServer();
    void stopServer();
    void restartServer();

protected:

    class InputDevice : public controller::InputDevice {
    public:
        friend class OscPlugin;

        InputDevice() : controller::InputDevice("OSC") {}

        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        virtual void focusOutEvent() override {};

        void clearState();
        void setContainer(OscPlugin* container) { _container = container; }

        OscPlugin* _container { nullptr };
    };

    std::shared_ptr<InputDevice> _inputDevice;

    static const char* NAME;
    static const char* OSC_ID_STRING;

    bool _enabled { false };
    mutable bool _initialized { false };

    lo_server_thread _oscServerThread { nullptr };

public:
    bool _debug { false };
    enum Constants { DEFAULT_OSC_SERVER_PORT = 7700 };
    int _serverPort { DEFAULT_OSC_SERVER_PORT };
    controller::InputCalibrationData _lastInputCalibrationData;

    std::vector<float> _blendshapeValues;
    std::vector<bool> _blendshapeValidFlags;
    glm::quat _headRot;
    bool _headRotValid { false };
    glm::vec3 _headTransTarget;
    glm::vec3 _headTransSmoothed;
    bool _headTransValid { false };
    glm::quat _eyeLeftRot;
    bool _eyeLeftRotValid { false };
    glm::quat _eyeRightRot;
    bool _eyeRightRotValid { false };
    std::mutex _dataMutex;
};

#endif // hifi_OscPlugin_h

