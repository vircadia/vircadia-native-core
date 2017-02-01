//  SpacemouseManager.h
//  interface/src/devices
//
//  Created by Marcel Verhagen on 09-06-15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpacemouseManager_h
#define hifi_SpacemouseManager_h

#include <QObject>
#include <QLibrary>
#include <controllers/UserInputMapper.h>
#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>

#include <plugins/InputPlugin.h>

// the windows connexion rawinput
#ifdef Q_OS_WIN

#include "../../../interface/external/3dconnexionclient/include/I3dMouseParams.h"
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>
#include <Winsock2.h>
#include <windows.h>

// windows rawinput parameters
class MouseParameters : public I3dMouseParam {
public:
    MouseParameters();

    // I3dmouseSensor interface
    bool IsPanZoom() const;
    bool IsRotate() const;
    Speed GetSpeed() const;

    void SetPanZoom(bool isPanZoom);
    void SetRotate(bool isRotate);
    void SetSpeed(Speed speed);

    // I3dmouseNavigation interface
    Navigation GetNavigationMode() const;
    Pivot GetPivotMode() const;
    PivotVisibility GetPivotVisibility() const;
    bool IsLockHorizon() const;

    void SetLockHorizon(bool bOn);
    void SetNavigationMode(Navigation navigation);
    void SetPivotMode(Pivot pivot);
    void SetPivotVisibility(PivotVisibility visibility);

    static bool Is3dmouseAttached();
    
    

private:
    MouseParameters(const MouseParameters&);
    const MouseParameters& operator = (const MouseParameters&);

    Navigation fNavigation;
    Pivot fPivot;
    PivotVisibility fPivotVisibility;
    bool fIsLockHorizon;

    bool fIsPanZoom;
    bool fIsRotate;
    Speed fSpeed;
};

class SpacemouseManager : public InputPlugin, public QAbstractNativeEventFilter {
   
    Q_OBJECT
public:
    bool isSupported() const override;
    const QString getName() const override { return NAME; }
    const QString getID() const override { return NAME; }

    bool activate() override;
    void deactivate() override;

    void pluginFocusOutEvent() override;
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

private:
    void Move3d(HANDLE device, std::vector<float>& motionData);
    void On3dmouseKeyDown(HANDLE device, int virtualKeyCode);
    void On3dmouseKeyUp(HANDLE device, int virtualKeyCode);
    bool InitializeRawInput(HWND hwndTarget);

    bool RawInputEventFilter(void* msg, long* result);

    void OnRawInput(UINT nInputCode, HRAWINPUT hRawInput);
    UINT GetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader);
    bool TranslateRawInputData(UINT nInputCode, PRAWINPUT pRawInput);
    void On3dmouseInput();

    class TInputData {
    public:
        TInputData() : fAxes(6) {}

        bool IsZero() {
            return (0.0f == fAxes[0] && 0.0f == fAxes[1] && 0.0f == fAxes[2] &&
                0.0f == fAxes[3] && 0.0f == fAxes[4] && 0.0f == fAxes[5]);
        }

        int fTimeToLive; // For telling if the device was unplugged while sending data
        bool fIsDirty;
        std::vector<float> fAxes;

    };

    HWND fWindow;

    // Data cache to handle multiple rawinput devices
    std::map< HANDLE, TInputData> fDevice2Data;
    std::map< HANDLE, unsigned long> fDevice2Keystate;

    // 3dmouse parameters
    MouseParameters f3dMouseParams;     // Rotate, Pan Zoom etc.

    // use to calculate distance traveled since last event
    DWORD fLast3dmouseInputTime;

    static const char* NAME;
    friend class SpacemouseDevice;
};

// the osx connexion api
#else

#include <glm/glm.hpp>
#include "../../../interface/external/3dconnexionclient/include/ConnexionClientAPI.h"

class SpacemouseManager : public QObject {
    Q_OBJECT
public:
    void init();
    void destroy();
    bool Is3dmouseAttached();
    public slots:
    void toggleSpacemouse(bool shouldEnable);
};

#endif // __APPLE__


// connnects to the userinputmapper
class SpacemouseDevice : public QObject, public controller::InputDevice {
    Q_OBJECT

public:
    SpacemouseDevice();
    enum PositionChannel {
        TRANSLATE_X,
        TRANSLATE_Y,
        TRANSLATE_Z,
        ROTATE_X,
        ROTATE_Y,
        ROTATE_Z,
    };

    enum ButtonChannel {
        BUTTON_1 = 1,
        BUTTON_2 = 2,
        BUTTON_3 = 3
    };

    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;

    float getButton(int channel) const;
    float getAxis(int channel) const;

    controller::Input makeInput(SpacemouseDevice::PositionChannel axis) const;
    controller::Input makeInput(SpacemouseDevice::ButtonChannel button) const;

    controller::Input::NamedPair makePair(SpacemouseDevice::PositionChannel axis, const QString& name) const;
    controller::Input::NamedPair makePair(SpacemouseDevice::ButtonChannel button, const QString& name) const;

    virtual controller::Input::NamedVector getAvailableInputs() const override;
    virtual QString getDefaultMappingConfig() const override;
    virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
    virtual void focusOutEvent() override;

    glm::vec3 cc_position;
    glm::vec3 cc_rotation;
    int clientId;

    void setButton(int lastButtonState);
    void handleAxisEvent();
};

#endif // defined(hifi_SpacemouseManager_h)
