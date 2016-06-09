//
//  Created by Bradley Austin Davis on 2015/11/01
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrHelpers.h"

#include <atomic>
#include <mutex>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QProcessEnvironment>
#include <QtGui/QInputMethodEvent>
#include <QtQuick/QQuickWindow>

#include <Windows.h>

#include <OffscreenUi.h>

Q_DECLARE_LOGGING_CATEGORY(displayplugins)
Q_LOGGING_CATEGORY(displayplugins, "hifi.plugins.display")

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

static int refCount { 0 };
static Mutex mutex;
static vr::IVRSystem* activeHmd { nullptr };
static bool _openVrQuitRequested { false };

bool openVrQuitRequested() {
    return _openVrQuitRequested;
}

static const uint32_t RELEASE_OPENVR_HMD_DELAY_MS = 5000;

bool isOculusPresent() {
    bool result = false;
#if defined(Q_OS_WIN32) 
    HANDLE oculusServiceEvent = ::OpenEventW(SYNCHRONIZE, FALSE, L"OculusHMDConnected");
    // The existence of the service indicates a running Oculus runtime
    if (oculusServiceEvent) {
        // A signaled event indicates a connected HMD
        if (WAIT_OBJECT_0 == ::WaitForSingleObject(oculusServiceEvent, 0)) {
            result = true;
        }
        ::CloseHandle(oculusServiceEvent);
    }
#endif 
    return result;
}

bool openVrSupported() {
    static const QString DEBUG_FLAG("HIFI_DEBUG_OPENVR");
    static bool enableDebugOpenVR = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
    return (enableDebugOpenVR || !isOculusPresent()) && vr::VR_IsHmdPresent();
}

vr::IVRSystem* acquireOpenVrSystem() {
    bool hmdPresent = vr::VR_IsHmdPresent();
    if (hmdPresent) {
        Lock lock(mutex);
        if (!activeHmd) {
            qCDebug(displayplugins) << "OpenVR: No vr::IVRSystem instance active, building";
            vr::EVRInitError eError = vr::VRInitError_None;
            activeHmd = vr::VR_Init(&eError, vr::VRApplication_Scene);
            qCDebug(displayplugins) << "OpenVR display: HMD is " << activeHmd << " error is " << eError;
        }
        if (activeHmd) {
            qCDebug(displayplugins) << "OpenVR: incrementing refcount";
            ++refCount;
        }
    } else {
        qCDebug(displayplugins) << "OpenVR: no hmd present";
    }
    return activeHmd;
}

void releaseOpenVrSystem() {
    if (activeHmd) {
        Lock lock(mutex);
        qCDebug(displayplugins) << "OpenVR: decrementing refcount";
        --refCount;
        if (0 == refCount) {
            qCDebug(displayplugins) << "OpenVR: zero refcount, deallocate VR system";
            vr::VR_Shutdown();
            _openVrQuitRequested = false;
            activeHmd = nullptr;
        }
    }
}

static char textArray[8192];

static QMetaObject::Connection _focusConnection, _focusTextConnection;
extern bool _openVrDisplayActive;
static vr::IVROverlay* _overlay { nullptr };
static QObject* _keyboardFocusObject { nullptr };
static QString _existingText;
static Qt::InputMethodHints _currentHints;
extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
static bool _keyboardShown { false };
static const uint32_t SHOW_KEYBOARD_DELAY_MS = 100;

void showOpenVrKeyboard(bool show = true) {
    if (!_overlay) {
        return;
    }

    if (show) {
        // To avoid flickering the keyboard when a text element is only briefly selected, 
        // show the keyboard asynchrnously after a very short delay, but only after we check 
        // that the current focus object is still one that is text enabled
        QTimer::singleShot(SHOW_KEYBOARD_DELAY_MS, [] {
            auto offscreenUi = DependencyManager::get<OffscreenUi>();
            auto currentFocus = offscreenUi->getWindow()->focusObject();
            QInputMethodQueryEvent query(Qt::ImEnabled | Qt::ImQueryInput | Qt::ImHints);
            qApp->sendEvent(currentFocus, &query);
            // Current focus isn't text enabled, bail early.
            if (!query.value(Qt::ImEnabled).toBool()) {
                return;
            }
            // We're going to show the keyboard now...
            _keyboardFocusObject = currentFocus;
            _currentHints = Qt::InputMethodHints(query.value(Qt::ImHints).toUInt());
            vr::EGamepadTextInputMode inputMode = vr::k_EGamepadTextInputModeNormal;
            if (_currentHints & Qt::ImhHiddenText) {
                inputMode = vr::k_EGamepadTextInputModePassword;
            }
            vr::EGamepadTextInputLineMode lineMode = vr::k_EGamepadTextInputLineModeSingleLine;
            if (_currentHints & Qt::ImhMultiLine) {
                lineMode = vr::k_EGamepadTextInputLineModeMultipleLines;
            }
            _existingText = query.value(Qt::ImSurroundingText).toString();

            auto showKeyboardResult = _overlay->ShowKeyboard(inputMode, lineMode, "Keyboard", 1024,
                _existingText.toLocal8Bit().toStdString().c_str(), false, 0);

            if (vr::VROverlayError_None == showKeyboardResult) {
                _keyboardShown = true;
                // Try to position the keyboard slightly below where the user is looking.
                mat4 headPose = toGlm(_trackedDevicePose[0].mDeviceToAbsoluteTracking);
                mat4 keyboardTransform = glm::translate(headPose, vec3(0, -0.5, -1));
                keyboardTransform = keyboardTransform * glm::rotate(mat4(), 3.14159f / 4.0f, vec3(-1, 0, 0));
                auto keyboardTransformVr = toOpenVr(keyboardTransform);
                _overlay->SetKeyboardTransformAbsolute(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, &keyboardTransformVr);
            }
        });
    } else {
        _keyboardFocusObject = nullptr;
        if (_keyboardShown) {
            _overlay->HideKeyboard();
            _keyboardShown = false;
        }
    }
}

void finishOpenVrKeyboardInput() {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto chars = _overlay->GetKeyboardText(textArray, 8192);
    auto newText = QString(QByteArray(textArray, chars));
    // TODO modify the new text to match the possible input hints:
    // ImhDigitsOnly  ImhFormattedNumbersOnly  ImhUppercaseOnly  ImhLowercaseOnly 
    // ImhDialableCharactersOnly ImhEmailCharactersOnly  ImhUrlCharactersOnly  ImhLatinOnly
    QInputMethodEvent event(_existingText, QList<QInputMethodEvent::Attribute>());
    event.setCommitString(newText, 0, _existingText.size());
    qApp->sendEvent(_keyboardFocusObject, &event);
    // Simulate an enter press on the top level window to trigger the action
    if (0 == (_currentHints & Qt::ImhMultiLine)) {
        qApp->sendEvent(offscreenUi->getWindow(), &QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::KeyboardModifiers(), QString("\n")));
        qApp->sendEvent(offscreenUi->getWindow(), &QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::KeyboardModifiers()));
    }
}

static const QString DEBUG_FLAG("HIFI_DISABLE_STEAM_VR_KEYBOARD");
bool disableSteamVrKeyboard = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);

void enableOpenVrKeyboard() {
    if (disableSteamVrKeyboard) {
        return;
    }
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    _overlay = vr::VROverlay();

    _focusConnection = QObject::connect(offscreenUi->getWindow(), &QQuickWindow::focusObjectChanged, [](QObject* object) {
        if (object != _keyboardFocusObject) {
            showOpenVrKeyboard(false);
        }
    });

    _focusTextConnection = QObject::connect(offscreenUi.data(), &OffscreenUi::focusTextChanged, [](bool focusText) {
        if (_openVrDisplayActive) {
            showOpenVrKeyboard(focusText);
        }
    });
}


void disableOpenVrKeyboard() {
    if (disableSteamVrKeyboard) {
        return;
    }
    QObject::disconnect(_focusTextConnection);
    QObject::disconnect(_focusConnection);
}

bool isOpenVrKeyboardShown() {
    return _keyboardShown;
}


void handleOpenVrEvents() {
    if (!activeHmd) {
        return;
    }
    Lock lock(mutex);
    if (!activeHmd) {
        return;
    }

    vr::VREvent_t event;
    while (activeHmd->PollNextEvent(&event, sizeof(event))) {
        switch (event.eventType) {
            case vr::VREvent_Quit:
                _openVrQuitRequested = true;
                activeHmd->AcknowledgeQuit_Exiting();
                break;

            case vr::VREvent_KeyboardDone: 
                finishOpenVrKeyboardInput();
                break;

            default:
                break;
        }
        qDebug() << "OpenVR: Event " << event.eventType;
    }

}

