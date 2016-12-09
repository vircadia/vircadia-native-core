//
//  KinectPlugin.h
//
//  Created by Brad Hefta-Gaub on 2016/12/7
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_KinectPlugin_h
#define hifi_KinectPlugin_h

#ifdef HAVE_KINECT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif

// Windows Header Files
#include <windows.h>

#include <Shlobj.h>

// Kinect Header files
#include <Kinect.h>

// Safe release for interfaces
template<class Interface> inline void SafeRelease(Interface *& pInterfaceToRelease) {
    if (pInterfaceToRelease != NULL) {
        pInterfaceToRelease->Release();
        pInterfaceToRelease = NULL;
    }
}
#endif

#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>
#include <plugins/InputPlugin.h>

// Handles interaction with the Kinect SDK
class KinectPlugin : public InputPlugin {
    Q_OBJECT
public:
    //friend void FrameDataReceivedCallback(void* context, void* sender, _BvhDataHeaderEx* header, float* data);

    bool isHandController() const override { return false; }

    // Plugin functions
    virtual void init() override;
    virtual bool isSupported() const override;
    virtual const QString getName() const override { return NAME; }
    const QString getID() const override { return KINECT_ID_STRING; }

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

protected:

    struct KinectJoint {
        glm::vec3 position;
        glm::quat orientation;
    };

    class InputDevice : public controller::InputDevice {
    public:
        friend class KinectPlugin;

        InputDevice() : controller::InputDevice("Kinect") {}

        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override {};
        virtual void focusOutEvent() override {};

        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, 
                const std::vector<KinectPlugin::KinectJoint>& joints, const std::vector<KinectPlugin::KinectJoint>& prevJoints);
    };

    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

    static const char* NAME;
    static const char* KINECT_ID_STRING;

    bool _enabled;

    // copy of data directly from the KinectDataReader SDK
    std::vector<KinectJoint> _joints;

    // one frame old copy of _joints, used to caluclate angular and linear velocity.
    std::vector<KinectJoint> _prevJoints;


    // Kinect SDK related items...

    bool KinectPlugin::initializeDefaultSensor();
    void updateBody();

#ifdef HAVE_KINECT
    void ProcessBody(INT64 time, int bodyCount, IBody** bodies);

    // Current Kinect
    IKinectSensor*          _kinectSensor { nullptr };
    ICoordinateMapper*      _coordinateMapper { nullptr };

    // Body reader
    IBodyFrameReader*       _bodyFrameReader { nullptr };
#endif

};

#endif // hifi_KinectPlugin_h

