//
//  NeuronPlugin.h
//  input-plugins/src/input-plugins
//
//  Created by Anthony Thibault on 12/18/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NeuronPlugin_h
#define hifi_NeuronPlugin_h

#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>
#include <plugins/InputPlugin.h>

struct _BvhDataHeaderEx;
void FrameDataReceivedCallback(void* context, void* sender, _BvhDataHeaderEx* header, float* data);

// Handles interaction with the Neuron SDK
class NeuronPlugin : public InputPlugin {
    Q_OBJECT
public:
    friend void FrameDataReceivedCallback(void* context, void* sender, _BvhDataHeaderEx* header, float* data);

    bool isHandController() const override { return false; }

    // Plugin functions
    virtual bool isSupported() const override;
    virtual const char* getName() const override { return NAME; }
    const QString& getID() const override { return NEURON_ID_STRING; }

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

protected:

    struct NeuronJoint {
        glm::vec3 pos;
        glm::vec3 euler;
    };

    class InputDevice : public controller::InputDevice {
    public:
        friend class NeuronPlugin;

        InputDevice() : controller::InputDevice("Neuron") {}

        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override {};
        virtual void focusOutEvent() override {};

        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, const std::vector<NeuronPlugin::NeuronJoint>& joints, const std::vector<NeuronPlugin::NeuronJoint>& prevJoints);
    };

    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

    static const char* NAME;
    static const char* NEURON_ID_STRING;

    std::string _serverAddress;
    int _serverPort;
    void* _socketRef;

    // used to guard multi-threaded access to _joints
    std::mutex _jointsMutex;

    // copy of data directly from the NeuronDataReader SDK
    std::vector<NeuronJoint> _joints;

    // one frame old copy of _joints, used to caluclate angular and linear velocity.
    std::vector<NeuronJoint> _prevJoints;
};

#endif // hifi_NeuronPlugin_h

