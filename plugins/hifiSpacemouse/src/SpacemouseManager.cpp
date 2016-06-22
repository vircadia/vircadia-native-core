//
//  SpacemouseManager.cpp
//  interface/src/devices
//
//  Created by MarcelEdward Verhagen on 09-06-15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SpacemouseManager.h"

#ifdef Q_OS_WIN
#include <VersionHelpers.h>
#endif

#include <UserActivityLogger.h>
#include <PathUtils.h>

#include <controllers/UserInputMapper.h>

const QString SpacemouseManager::NAME { "Spacemouse" };

const float MAX_AXIS = 75.0f;  // max forward = 2x speed
#define LOGITECH_VENDOR_ID 0x46d

#ifndef RIDEV_DEVNOTIFY
#define RIDEV_DEVNOTIFY 0x00002000
#endif

const int TRACE_RIDI_DEVICENAME = 0;
const int TRACE_RIDI_DEVICEINFO = 0;

#ifdef _WIN64
typedef unsigned __int64 QWORD;
#endif

bool Is3dmouseAttached();

std::shared_ptr<SpacemouseDevice> instance;

bool SpacemouseManager::isSupported() const {
    return Is3dmouseAttached();
}

bool SpacemouseManager::activate() {
    fLast3dmouseInputTime = 0;

    InitializeRawInput(GetActiveWindow());

    QAbstractEventDispatcher::instance()->installNativeEventFilter(this);

    if (!instance) {
        instance = std::make_shared<SpacemouseDevice>();
    }

    if (instance->getDeviceID() == controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->registerDevice(instance);
    }
    return true;
}

void SpacemouseManager::deactivate() {
    QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);
    int deviceid = instance->getDeviceID();
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    userInputMapper->removeDevice(deviceid);
}

void SpacemouseManager::pluginFocusOutEvent() { 
    instance->focusOutEvent(); 
}

void SpacemouseManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    
}

SpacemouseDevice::SpacemouseDevice() : InputDevice(SpacemouseManager::NAME)
{
}

void SpacemouseDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};


void SpacemouseDevice::handleAxisEvent() {
    auto rotation = cc_rotation / MAX_AXIS;
    _axisStateMap[ROTATE_X] = rotation.x;
    _axisStateMap[ROTATE_Y] = rotation.y;
    _axisStateMap[ROTATE_Z] = rotation.z;
    auto position = cc_position / MAX_AXIS;
    _axisStateMap[TRANSLATE_X] = position.x;
    _axisStateMap[TRANSLATE_Y] = position.y;
    _axisStateMap[TRANSLATE_Z] = position.z;
}

void SpacemouseDevice::setButton(int lastButtonState) {
    _buttonPressedMap.clear();
    _buttonPressedMap.insert(lastButtonState);
}


controller::Input::NamedVector SpacemouseDevice::getAvailableInputs() const {
    using namespace controller;


    static const Input::NamedVector availableInputs{

        makePair(BUTTON_1, "LeftButton"),
        makePair(BUTTON_2, "RightButton"),
        //makePair(BUTTON_3, "BothButtons"),
        makePair(TRANSLATE_X, "TranslateX"),
        makePair(TRANSLATE_Y, "TranslateY"),
        makePair(TRANSLATE_Z, "TranslateZ"),
        //makePair(ROTATE_X, "RotateX"),
        //makePair(ROTATE_Y, "RotateY"),
        makePair(ROTATE_Z, "RotateZ"),

    };
    return availableInputs;
}

QString SpacemouseDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/spacemouse.json";
    return MAPPING_JSON;
}

float SpacemouseDevice::getButton(int channel) const {
    if (!_buttonPressedMap.empty()) {
        if (_buttonPressedMap.find(channel) != _buttonPressedMap.end()) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    return 0.0f;
}

float SpacemouseDevice::getAxis(int channel) const {
    auto axis = _axisStateMap.find(channel);
    if (axis != _axisStateMap.end()) {
        return (*axis).second;
    } else {
        return 0.0f;
    }
}

controller::Input SpacemouseDevice::makeInput(SpacemouseDevice::ButtonChannel button) const {
    return controller::Input(_deviceID, button, controller::ChannelType::BUTTON);
}

controller::Input SpacemouseDevice::makeInput(SpacemouseDevice::PositionChannel axis) const {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input::NamedPair SpacemouseDevice::makePair(SpacemouseDevice::ButtonChannel button, const QString& name) const {
    return controller::Input::NamedPair(makeInput(button), name);
}

controller::Input::NamedPair SpacemouseDevice::makePair(SpacemouseDevice::PositionChannel axis, const QString& name) const {
    return controller::Input::NamedPair(makeInput(axis), name);
}

void SpacemouseDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    // the update is done in the SpacemouseManager class.
    // for windows in the nativeEventFilter the inputmapper is connected or registed or removed when an 3Dconnnexion device is attached or detached
    // for osx the api will call DeviceAddedHandler or DeviceRemoveHandler when a 3Dconnexion device is attached or detached
}

#ifdef Q_OS_WIN

bool SpacemouseManager::nativeEventFilter(const QByteArray& eventType, void* message, long* result) {
    MSG* msg = static_cast< MSG * >(message);
    return RawInputEventFilter(message, result);
}


//Get an initialized array of PRAWINPUTDEVICE for the 3D devices
//pNumDevices returns the number of devices to register. Currently this is always 1.
static PRAWINPUTDEVICE GetDevicesToRegister(unsigned int* pNumDevices) {
    // Array of raw input devices to register
    static RAWINPUTDEVICE sRawInputDevices[] = {
        { 0x01, 0x08, 0x00, 0x00 } // Usage Page = 0x01 Generic Desktop Page, Usage Id= 0x08 Multi-axis Controller
    };

    if (pNumDevices) {
        *pNumDevices = sizeof(sRawInputDevices) / sizeof(sRawInputDevices[0]);
    }

    return sRawInputDevices;
}


//Detect the 3D mouse
bool Is3dmouseAttached() {
    unsigned int numDevicesOfInterest = 0;
    PRAWINPUTDEVICE devicesToRegister = GetDevicesToRegister(&numDevicesOfInterest);

    unsigned int nDevices = 0;

    if (::GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {
        return false;
    }

    if (nDevices == 0) {
        return false;
    }

    std::vector<RAWINPUTDEVICELIST> rawInputDeviceList(nDevices);
    if (::GetRawInputDeviceList(&rawInputDeviceList[0], &nDevices, sizeof(RAWINPUTDEVICELIST)) == static_cast<unsigned int>(-1)) {
        return false;
    }

    for (unsigned int i = 0; i < nDevices; ++i) {
        RID_DEVICE_INFO rdi = { sizeof(rdi) };
        unsigned int cbSize = sizeof(rdi);

        if (GetRawInputDeviceInfo(rawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &rdi, &cbSize) > 0) {
            //skip non HID and non logitec (3DConnexion) devices
            if (rdi.dwType != RIM_TYPEHID || rdi.hid.dwVendorId != LOGITECH_VENDOR_ID) {
                continue;
            }

            //check if devices matches Multi-axis Controller
            for (unsigned int j = 0; j < numDevicesOfInterest; ++j) {
                if (devicesToRegister[j].usUsage == rdi.hid.usUsage
                    && devicesToRegister[j].usUsagePage == rdi.hid.usUsagePage) {
                    return true;
                }
            }
        }
    }
    return false;
}

// object angular velocity per mouse tick 0.008 milliradians per second per count
static const double k3dmouseAngularVelocity = 8.0e-6; // radians per second per count

static const int kTimeToLive = 5;

enum ConnexionPid {
    CONNEXIONPID_SPACEPILOT = 0xc625,
    CONNEXIONPID_SPACENAVIGATOR = 0xc626,
    CONNEXIONPID_SPACEEXPLORER = 0xc627,
    CONNEXIONPID_SPACENAVIGATORFORNOTEBOOKS = 0xc628,
    CONNEXIONPID_SPACEPILOTPRO = 0xc629
};

// e3dmouse_virtual_key
enum V3dk {
    V3DK_INVALID = 0,
    V3DK_MENU = 1, V3DK_FIT,
    V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT, V3DK_BOTTOM, V3DK_BACK,
    V3DK_CW, V3DK_CCW,
    V3DK_ISO1, V3DK_ISO2,
    V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6, V3DK_7, V3DK_8, V3DK_9, V3DK_10,
    V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL,
    V3DK_ROTATE, V3DK_PANZOOM, V3DK_DOMINANT,
    V3DK_PLUS, V3DK_MINUS
};

struct tag_VirtualKeys {
    ConnexionPid pid;
    size_t nKeys;
    V3dk *vkeys;
};

// e3dmouse_virtual_key
static const V3dk SpaceExplorerKeys[] = {
    V3DK_INVALID, // there is no button 0
    V3DK_1, V3DK_2,
    V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT,
    V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL,
    V3DK_FIT, V3DK_MENU,
    V3DK_PLUS, V3DK_MINUS,
    V3DK_ROTATE
};

//e3dmouse_virtual_key
static const V3dk SpacePilotKeys[] = {
    V3DK_INVALID,
    V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6,
    V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT,
    V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL,
    V3DK_FIT, V3DK_MENU,
    V3DK_PLUS, V3DK_MINUS,
    V3DK_DOMINANT, V3DK_ROTATE,
};

static const struct tag_VirtualKeys _3dmouseVirtualKeys[] = {
    CONNEXIONPID_SPACEPILOT,
    sizeof(SpacePilotKeys) / sizeof(SpacePilotKeys[0]),
    const_cast<V3dk *>(SpacePilotKeys),
    CONNEXIONPID_SPACEEXPLORER,
    sizeof(SpaceExplorerKeys) / sizeof(SpaceExplorerKeys[0]),
    const_cast<V3dk *>(SpaceExplorerKeys)
};

// Converts a hid device keycode (button identifier) of a pre-2009 3Dconnexion USB device to the standard 3d mouse virtual key definition.
// pid USB Product ID (PID) of 3D mouse device
// hidKeyCode  Hid keycode as retrieved from a Raw Input packet
// return The standard 3d mouse virtual key (button identifier) or zero if an error occurs.

// Converts a hid device keycode (button identifier) of a pre-2009 3Dconnexion USB device
// to the standard 3d mouse virtual key definition.
unsigned short HidToVirtualKey(unsigned long pid, unsigned short hidKeyCode) {
    unsigned short virtualkey = hidKeyCode;
    for (size_t i = 0; i<sizeof(_3dmouseVirtualKeys) / sizeof(_3dmouseVirtualKeys[0]); ++i) {
        if (pid == _3dmouseVirtualKeys[i].pid) {
            if (hidKeyCode < _3dmouseVirtualKeys[i].nKeys) {
                virtualkey = _3dmouseVirtualKeys[i].vkeys[hidKeyCode];
            } else {
                virtualkey = V3DK_INVALID;
            }
            break;
        }
    }
    // Remaining devices are unchanged
    return virtualkey;
}

bool SpacemouseManager::RawInputEventFilter(void* msg, long* result) {

    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    if (Is3dmouseAttached() && instance->getDeviceID() == controller::Input::INVALID_DEVICE) {
        userInputMapper->registerDevice(instance);
    }
    else if (!Is3dmouseAttached() && instance->getDeviceID() != controller::Input::INVALID_DEVICE) {
        userInputMapper->removeDevice(instance->getDeviceID());
    }

    if (!Is3dmouseAttached()) {
        return false;
    }

    MSG* message = (MSG*)(msg);

    if (message->message == WM_INPUT) {
        HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(message->lParam);
        OnRawInput(RIM_INPUT, hRawInput);
        if (result != 0) {
            result = 0;
        }
        return true;
    }
    return false;
}

//Called with the processed motion data when a 3D mouse event is received
void SpacemouseManager::Move3d(HANDLE device, std::vector<float>& motionData) {
    Q_UNUSED(device);
    instance->cc_position = { motionData[0] * 1000, motionData[1] * 1000, motionData[2] * 1000 };
    instance->cc_rotation = { motionData[3] * 1500, motionData[4] * 1500, motionData[5] * 1500 };
    instance->handleAxisEvent();
}

//Called when a 3D mouse key is pressed
void SpacemouseManager::On3dmouseKeyDown(HANDLE device, int virtualKeyCode) {
    Q_UNUSED(device);
    instance->setButton(virtualKeyCode);
}

//Called when a 3D mouse key is released
void SpacemouseManager::On3dmouseKeyUp(HANDLE device, int virtualKeyCode) {
    Q_UNUSED(device);
    instance->setButton(0);
}


// Initialize the window to recieve raw-input messages
// This needs to be called initially so that Windows will send the messages from the 3D mouse to the window.
bool SpacemouseManager::InitializeRawInput(HWND hwndTarget) {
    fWindow = hwndTarget;

    // Simply fail if there is no window
    if (!hwndTarget) {
        return false;
    }

    unsigned int numDevices = 0;
    PRAWINPUTDEVICE devicesToRegister = GetDevicesToRegister(&numDevices);

    if (numDevices == 0) {
        return false;
    }

    unsigned int cbSize = sizeof(devicesToRegister[0]);
    for (size_t i = 0; i < numDevices; i++) {
        // Set the target window to use
        //devicesToRegister[i].hwndTarget = hwndTarget;

        // If Vista or newer, enable receiving the WM_INPUT_DEVICE_CHANGE message.
        if (IsWindowsVistaOrGreater()) {
            devicesToRegister[i].dwFlags |= RIDEV_DEVNOTIFY;
        }
    }
    return (::RegisterRawInputDevices(devicesToRegister, numDevices, cbSize) != FALSE);
}

//Get the raw input data from Windows
UINT SpacemouseManager::GetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader) {
    //Includes workaround for incorrect alignment of the RAWINPUT structure on x64 os
    //when running as Wow64 (copied directly from 3DConnexion code)
#ifdef _WIN64
    return ::GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
#else
    BOOL bIsWow64 = FALSE;
    ::IsWow64Process(GetCurrentProcess(), &bIsWow64);
    if (!bIsWow64 || pData == NULL) {
        return ::GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
    } else {
        HWND hwndTarget = fWindow;

        size_t cbDataSize = 0;
        UINT nCount = 0;
        PRAWINPUT pri = pData;

        MSG msg;
        while (PeekMessage(&msg, hwndTarget, WM_INPUT, WM_INPUT, PM_NOREMOVE)) {
            HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(msg.lParam);
            size_t cbSize = *pcbSize - cbDataSize;
            if (::GetRawInputData(hRawInput, RID_INPUT, pri, &cbSize, cbSizeHeader) == static_cast<UINT>(-1)) {
                if (nCount == 0) {
                    return static_cast<UINT>(-1);
                } else {
                    break;
                }
            }
            ++nCount;

            // Remove the message for the data just read
            PeekMessage(&msg, hwndTarget, WM_INPUT, WM_INPUT, PM_REMOVE);

            pri = NEXTRAWINPUTBLOCK(pri);
            cbDataSize = reinterpret_cast<ULONG_PTR>(pri)-reinterpret_cast<ULONG_PTR>(pData);
            if (cbDataSize >= *pcbSize) {
                cbDataSize = *pcbSize;
                break;
            }
        }
        return nCount;
    }
#endif
}

// Process the raw input device data
// On3dmouseInput() does all the preprocessing of the rawinput device data before
// finally calling the Move3d method.
void SpacemouseManager::On3dmouseInput() {
    // Don't do any data processing in background
    bool bIsForeground = (::GetActiveWindow() != NULL);
    if (!bIsForeground) {
        // set all cached data to zero so that a zero event is seen and the cached data deleted
        for (std::map<HANDLE, TInputData>::iterator it = fDevice2Data.begin(); it != fDevice2Data.end(); it++) {
            it->second.fAxes.assign(6, .0);
            it->second.fIsDirty = true;
        }
    }

    DWORD dwNow = ::GetTickCount();           // Current time;
    DWORD dwElapsedTime;                      // Elapsed time since we were last here

    if (0 == fLast3dmouseInputTime) {
        dwElapsedTime = 10;                    // System timer resolution
    } else {
        dwElapsedTime = dwNow - fLast3dmouseInputTime;
        if (fLast3dmouseInputTime > dwNow) {
            dwElapsedTime = ~dwElapsedTime + 1;
        }
        if (dwElapsedTime<1) {
            dwElapsedTime = 1;
        } else if (dwElapsedTime > 500) {
            // Check for wild numbers because the device was removed while sending data
            dwElapsedTime = 10;
        }
    }

    //qDebug("On3DmouseInput() period is %dms\n", dwElapsedTime);

    float mouseData2Rotation = k3dmouseAngularVelocity;
    // v = w * r,  we don't know r yet so lets assume r=1.)
    float mouseData2PanZoom = k3dmouseAngularVelocity;

    // Grab the I3dmouseParam interface
    I3dMouseParam& i3dmouseParam = f3dMouseParams;
    // Take a look at the users preferred speed setting and adjust the sensitivity accordingly
    I3dMouseSensor::Speed speedSetting = i3dmouseParam.GetSpeed();
    // See "Programming for the 3D Mouse", Section 5.1.3
    float speed = (speedSetting == I3dMouseSensor::SPEED_LOW ? 0.25f : speedSetting == I3dMouseSensor::SPEED_HIGH ? 4.f : 1.f);

    // Multiplying by the following will convert the 3d mouse data to real world units
    mouseData2PanZoom *= speed;
    mouseData2Rotation *= speed;

    std::map<HANDLE, TInputData>::iterator iterator = fDevice2Data.begin();
    while (iterator != fDevice2Data.end()) {

        // If we have not received data for a while send a zero event
        if ((--(iterator->second.fTimeToLive)) == 0) {
            iterator->second.fAxes.assign(6, .0);
        } else if (!iterator->second.fIsDirty) { //!t_bPoll3dmouse &&
            // If we are not polling then only handle the data that was actually received
            ++iterator;
            continue;
        }
        iterator->second.fIsDirty = false;

        // get a copy of the device
        HANDLE hdevice = iterator->first;

        // get a copy of the motion vectors and apply the user filters
        std::vector<float> motionData = iterator->second.fAxes;

        // apply the user filters

        // Pan Zoom filter
        // See "Programming for the 3D Mouse", Section 5.1.2
        if (!i3dmouseParam.IsPanZoom()) {
        // Pan zoom is switched off so set the translation vector values to zero
            motionData[0] = motionData[1] = motionData[2] = 0.;
        }

        // Rotate filter
        // See "Programming for the 3D Mouse", Section 5.1.1
        if (!i3dmouseParam.IsRotate()) {
        // Rotate is switched off so set the rotation vector values to zero
            motionData[3] = motionData[4] = motionData[5] = 0.;
        }

        // convert the translation vector into physical data
        for (int axis = 0; axis < 3; axis++) {
            motionData[axis] *= mouseData2PanZoom;
        }

        // convert the directed Rotate vector into physical data
        // See "Programming for the 3D Mouse", Section 7.2.2
        for (int axis = 3; axis < 6; axis++) {
            motionData[axis] *= mouseData2Rotation;
        }

        // Now that the data has had the filters and sensitivty settings applied
        // calculate the displacements since the last view update
        for (int axis = 0; axis < 6; axis++) {
            motionData[axis] *= dwElapsedTime;
        }

        // Now a bit of book keeping before passing on the data
        if (iterator->second.IsZero()) {
            iterator = fDevice2Data.erase(iterator);
        } else {
            ++iterator;
        }

        // Work out which will be the next device
        HANDLE hNextDevice = 0;
        if (iterator != fDevice2Data.end()) {
            hNextDevice = iterator->first;
        }

        // Pass the 3dmouse input to the view controller
        Move3d(hdevice, motionData);

        // Because we don't know what happened in the previous call, the cache might have
        // changed so reload the iterator
        iterator = fDevice2Data.find(hNextDevice);
    }

    if (!fDevice2Data.empty()) {
        fLast3dmouseInputTime = dwNow;
    } else {
        fLast3dmouseInputTime = 0;
    }
}

//Called when new raw input data is available
void SpacemouseManager::OnRawInput(UINT nInputCode, HRAWINPUT hRawInput) {
    const size_t cbSizeOfBuffer = 1024;
    BYTE pBuffer[cbSizeOfBuffer];

    PRAWINPUT pRawInput = reinterpret_cast<PRAWINPUT>(pBuffer);
    UINT cbSize = cbSizeOfBuffer;

    if (::GetRawInputData(hRawInput, RID_INPUT, pRawInput, &cbSize, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
        return;
    }

    bool b3dmouseInput = TranslateRawInputData(nInputCode, pRawInput);
    ::DefRawInputProc(&pRawInput, 1, sizeof(RAWINPUTHEADER));

    // Check for any buffered messages
    cbSize = cbSizeOfBuffer;
    UINT nCount = this->GetRawInputBuffer(pRawInput, &cbSize, sizeof(RAWINPUTHEADER));
    if (nCount == (UINT)-1) {
        qDebug("GetRawInputBuffer returned error %d\n", GetLastError());
    }

    while (nCount>0 && nCount != static_cast<UINT>(-1)) {
        PRAWINPUT pri = pRawInput;
        UINT nInput;
        for (nInput = 0; nInput<nCount; ++nInput) {
            b3dmouseInput |= TranslateRawInputData(nInputCode, pri);
            // clean the buffer
            ::DefRawInputProc(&pri, 1, sizeof(RAWINPUTHEADER));

            pri = NEXTRAWINPUTBLOCK(pri);
        }
        cbSize = cbSizeOfBuffer;
        nCount = this->GetRawInputBuffer(pRawInput, &cbSize, sizeof(RAWINPUTHEADER));
    }

    // If we have mouse input data for the app then tell tha app about it
    if (b3dmouseInput) {
        On3dmouseInput();
    }
}

bool SpacemouseManager::TranslateRawInputData(UINT nInputCode, PRAWINPUT pRawInput) {
    bool bIsForeground = (nInputCode == RIM_INPUT);

    // We are not interested in keyboard or mouse data received via raw input
    if (pRawInput->header.dwType != RIM_TYPEHID) {
        return false;
    }

    if (TRACE_RIDI_DEVICENAME == 1) {
        UINT dwSize = 0;
        if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, NULL, &dwSize) == 0) {
            std::vector<wchar_t> szDeviceName(dwSize + 1);
            if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, &szDeviceName[0], &dwSize) > 0) {
                qDebug("Device Name = %s\nDevice handle = 0x%x\n", &szDeviceName[0], pRawInput->header.hDevice);
            }
        }
    }

    RID_DEVICE_INFO sRidDeviceInfo;
    sRidDeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
    UINT cbSize = sizeof(RID_DEVICE_INFO);

    if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICEINFO, &sRidDeviceInfo, &cbSize) == cbSize) {
        if (TRACE_RIDI_DEVICEINFO == 1) {
            switch (sRidDeviceInfo.dwType) {
            case RIM_TYPEMOUSE:
                qDebug("\tsRidDeviceInfo.dwType=RIM_TYPEMOUSE\n");
                break;
            case RIM_TYPEKEYBOARD:
                qDebug("\tsRidDeviceInfo.dwType=RIM_TYPEKEYBOARD\n");
                break;
            case RIM_TYPEHID:
                qDebug("\tsRidDeviceInfo.dwType=RIM_TYPEHID\n");
                qDebug("\tVendor=0x%x\n\tProduct=0x%x\n\tUsagePage=0x%x\n\tUsage=0x%x\n",
                    sRidDeviceInfo.hid.dwVendorId,
                    sRidDeviceInfo.hid.dwProductId,
                    sRidDeviceInfo.hid.usUsagePage,
                    sRidDeviceInfo.hid.usUsage);
                break;
            }
        }

        if (sRidDeviceInfo.hid.dwVendorId == LOGITECH_VENDOR_ID) {
            if (pRawInput->data.hid.bRawData[0] == 0x01) { // Translation vector
                TInputData& deviceData = fDevice2Data[pRawInput->header.hDevice];
                deviceData.fTimeToLive = kTimeToLive;
                if (bIsForeground) {
                    short* pnRawData = reinterpret_cast<short*>(&pRawInput->data.hid.bRawData[1]);
                    // Cache the pan zoom data
                    deviceData.fAxes[0] = static_cast<float>(pnRawData[0]);
                    deviceData.fAxes[1] = static_cast<float>(pnRawData[1]);
                    deviceData.fAxes[2] = static_cast<float>(pnRawData[2]);

                    //qDebug("Pan/Zoom RI Data =\t0x%x,\t0x%x,\t0x%x\n", pnRawData[0], pnRawData[1], pnRawData[2]);

                    //if (pRawInput->data.hid.dwSizeHid >= 13) { // Highspeed package
                    //    // Cache the rotation data
                    //    deviceData.fAxes[3] = static_cast<float>(pnRawData[3]);
                    //    deviceData.fAxes[4] = static_cast<float>(pnRawData[4]);
                    //    deviceData.fAxes[5] = static_cast<float>(pnRawData[5]);
                    //    deviceData.fIsDirty = true;

                    //    qDebug("Rotation RI Data =\t0x%x,\t0x%x,\t0x%x\n", pnRawData[3], pnRawData[4], pnRawData[5]);
                    //    return true;
                    //}
                } else { // Zero out the data if the app is not in forground
                    deviceData.fAxes.assign(6, 0.f);
                }
            } else if (pRawInput->data.hid.bRawData[0] == 0x02) { // Rotation vector
                // If we are not in foreground do nothing
                // The rotation vector was zeroed out with the translation vector in the previous message
                if (bIsForeground) {
                    TInputData& deviceData = fDevice2Data[pRawInput->header.hDevice];
                    deviceData.fTimeToLive = kTimeToLive;

                    short* pnRawData = reinterpret_cast<short*>(&pRawInput->data.hid.bRawData[1]);
                    // Cache the rotation data
                    deviceData.fAxes[3] = static_cast<float>(pnRawData[0]);
                    deviceData.fAxes[4] = static_cast<float>(pnRawData[1]);
                    deviceData.fAxes[5] = static_cast<float>(pnRawData[2]);
                    deviceData.fIsDirty = true;

                    //qDebug("Rotation RI Data =\t0x%x,\t0x%x,\t0x%x\n", pnRawData[0], pnRawData[1], pnRawData[2]);

                    return true;
                }
            } else if (pRawInput->data.hid.bRawData[0] == 0x03) { // Keystate change
                // this is a package that contains 3d mouse keystate information
                // bit0=key1, bit=key2 etc.

                unsigned long dwKeystate = *reinterpret_cast<unsigned long*>(&pRawInput->data.hid.bRawData[1]);

                //qDebug("ButtonData =0x%x\n", dwKeystate);

                // Log the keystate changes
                unsigned long dwOldKeystate = fDevice2Keystate[pRawInput->header.hDevice];
                if (dwKeystate != 0) {
                    fDevice2Keystate[pRawInput->header.hDevice] = dwKeystate;
                } else {
                    fDevice2Keystate.erase(pRawInput->header.hDevice);
                }

                //  Only call the keystate change handlers if the app is in foreground
                if (bIsForeground) {
                    unsigned long dwChange = dwKeystate ^ dwOldKeystate;

                    for (int nKeycode = 1; nKeycode<33; nKeycode++) {
                        if (dwChange & 0x01) {
                            int nVirtualKeyCode = HidToVirtualKey(sRidDeviceInfo.hid.dwProductId, nKeycode);
                            if (nVirtualKeyCode) {
                                if (dwKeystate & 0x01) {
                                    On3dmouseKeyDown(pRawInput->header.hDevice, nVirtualKeyCode);
                                } else {
                                    On3dmouseKeyUp(pRawInput->header.hDevice, nVirtualKeyCode);
                                }
                            }
                        }
                        dwChange >>= 1;
                        dwKeystate >>= 1;
                    }
                }
            }
        }
    }
    return false;
}

MouseParameters::MouseParameters() :
    fNavigation(NAVIGATION_OBJECT_MODE),
    fPivot(PIVOT_AUTO),
    fPivotVisibility(PIVOT_SHOW),
    fIsLockHorizon(true),
    fIsPanZoom(true),
    fIsRotate(true),
    fSpeed(SPEED_LOW)
{
}

bool MouseParameters::IsPanZoom()  const {
    return fIsPanZoom;
}

bool MouseParameters::IsRotate()  const {
    return fIsRotate;
}

MouseParameters::Speed MouseParameters::GetSpeed()  const {
    return fSpeed;
}

void MouseParameters::SetPanZoom(bool isPanZoom) {
    fIsPanZoom = isPanZoom;
}

void MouseParameters::SetRotate(bool isRotate) {
    fIsRotate = isRotate;
}

void MouseParameters::SetSpeed(Speed speed) {
    fSpeed = speed;
}

MouseParameters::Navigation MouseParameters::GetNavigationMode() const {
    return fNavigation;
}

MouseParameters::Pivot MouseParameters::GetPivotMode() const {
    return fPivot;
}

MouseParameters::PivotVisibility MouseParameters::GetPivotVisibility() const {
    return fPivotVisibility;
}

bool MouseParameters::IsLockHorizon() const {
    return fIsLockHorizon;
}

void MouseParameters::SetLockHorizon(bool bOn) {
    fIsLockHorizon = bOn;
}

void MouseParameters::SetNavigationMode(Navigation navigation) {
    fNavigation = navigation;
}

void MouseParameters::SetPivotMode(Pivot pivot) {
    if (fPivot != PIVOT_MANUAL || pivot != PIVOT_AUTO_OVERRIDE) {
        fPivot = pivot;
    }
}

void MouseParameters::SetPivotVisibility(PivotVisibility visibility) {
    fPivotVisibility = visibility;
}

#else

int fConnexionClientID;

static SpacemouseDeviceState lastState;

static void DeviceAddedHandler(unsigned int connection);
static void DeviceRemovedHandler(unsigned int connection);
static void MessageHandler(unsigned int connection, unsigned int messageType, void *messageArgument);

void SpacemouseManager::toggleSpacemouse(bool shouldEnable) {
    if (shouldEnable && !Is3dmouseAttached()) {
        init();
    }
    if (!shouldEnable && Is3dmouseAttached()) {
        destroy();
    }
}

void SpacemouseManager::init() {
    // Make sure the framework is installed
    if (Menu::getInstance()->isOptionChecked(MenuOption::Connexion)) {
        // Install message handler and register our client
        InstallConnexionHandlers(MessageHandler, DeviceAddedHandler, DeviceRemovedHandler);
        // Either use this to take over in our application only... does not work
        // fConnexionClientID = RegisterConnexionClient('MCTt', "\pConnexion Client Test", kConnexionClientModeTakeOver, kConnexionMaskAll);

        // ...or use this to take over system-wide
        fConnexionClientID = RegisterConnexionClient(kConnexionClientWildcard, NULL, kConnexionClientModeTakeOver, kConnexionMaskAll);
        memcpy(&instance->clientId, &fConnexionClientID, (long)sizeof(int));

        // A separate API call is required to capture buttons beyond the first 8
        SetConnexionClientButtonMask(fConnexionClientID, kConnexionMaskAllButtons);

        // use default switches 
        ConnexionClientControl(fConnexionClientID, kConnexionCtlSetSwitches, kConnexionSwitchesDisabled, NULL);

        if (Is3dmouseAttached() && instance->getDeviceID() == controller::Input::INVALID_DEVICE) {
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            userInputMapper->registerDevice(instance);
            emit deviceConnected(getName());
        }
        //let one axis be dominant
        //ConnexionClientControl(fConnexionClientID, kConnexionCtlSetSwitches, kConnexionSwitchDominant | kConnexionSwitchEnableAll, NULL);
    }
}

void SpacemouseManager::destroy() {
    // Make sure the framework is installed
    if (&InstallConnexionHandlers != NULL) {
        // Unregister our client and clean up all handlers
        if (fConnexionClientID) {
            UnregisterConnexionClient(fConnexionClientID);
        }
        CleanupConnexionHandlers();
        fConnexionClientID = 0;
        if (instance->getDeviceID() != controller::Input::INVALID_DEVICE) {
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            userInputMapper->removeDevice(instance->getDeviceID());
            instance->setDeviceID(controller::Input::INVALID_DEVICE);
        }
    }
}

void DeviceAddedHandler(unsigned int connection) {
    if (instance->getDeviceID() == controller::Input::INVALID_DEVICE) {
        qCWarning(interfaceapp) << "Spacemouse device added ";
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->registerDevice(instance);
        UserActivityLogger::getInstance().connectedDevice("controller", "Spacemouse");
    }
}

void DeviceRemovedHandler(unsigned int connection) {
    if (instance->getDeviceID() != controller::Input::INVALID_DEVICE) {
        qCWarning(interfaceapp) << "Spacemouse device removed";
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->removeDevice(instance->getDeviceID());
        instance->setDeviceID(controller::Input::INVALID_DEVICE);
    }
}

bool SpacemouseManager::Is3dmouseAttached() {
    int result;
    if (fConnexionClientID) {
        if (ConnexionControl(kConnexionCtlGetDeviceID, 0, &result)) {
            return false;
        }
        return true;
    }
    return false;
}

void MessageHandler(unsigned int connection, unsigned int messageType, void *messageArgument) {
    SpacemouseDeviceState *state;

    switch (messageType) {
    case kConnexionMsgDeviceState:
        state = (SpacemouseDeviceState*)messageArgument;
        if (state->client == fConnexionClientID) {
            instance->cc_position = { state->axis[0], state->axis[1], state->axis[2] };
            instance->cc_rotation = { state->axis[3], state->axis[4], state->axis[5] };

            instance->handleAxisEvent();
            if (state->buttons != lastState.buttons) {
                instance->setButton(state->buttons);
            }
            memmove(&lastState, state, (long)sizeof(SpacemouseDeviceState));
        }
        break;
    case kConnexionMsgPrefsChanged:
        // the prefs have changed, do something
        break;
    default:
        // other messageTypes can happen and should be ignored
        break;
    }

}

#endif // __APPLE__
