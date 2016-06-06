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
static QObject* _focusObject { nullptr };
static QString _existingText;
static Qt::InputMethodHints _currentHints;

void showOpenVrKeyboard(bool show = true) {
    if (_overlay) {
        if (show) {
            auto offscreenUi = DependencyManager::get<OffscreenUi>();
            _focusObject = offscreenUi->getWindow()->focusObject();

            QInputMethodQueryEvent query(Qt::ImQueryInput | Qt::ImHints);
            qApp->sendEvent(_focusObject, &query);
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
            _overlay->ShowKeyboard(inputMode, lineMode, "Keyboard", 1024, _existingText.toLocal8Bit().toStdString().c_str(), false, (uint64_t)(void*)_focusObject);
        } else {
            _focusObject = nullptr;
            _overlay->HideKeyboard();
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
    qApp->sendEvent(_focusObject, &event);
    // Simulate an enter press on the top level window to trigger the action
    if (0 == (_currentHints & Qt::ImhMultiLine)) {
        qApp->sendEvent(offscreenUi->getWindow(), &QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::KeyboardModifiers(), QString("\n")));
        qApp->sendEvent(offscreenUi->getWindow(), &QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::KeyboardModifiers()));
    }
}

void enableOpenVrKeyboard() {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    _overlay = vr::VROverlay();

    _focusConnection = QObject::connect(offscreenUi->getWindow(), &QQuickWindow::focusObjectChanged, [](QObject* object) {
        if (object != _focusObject && _overlay) {
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
    QObject::disconnect(_focusTextConnection);
    QObject::disconnect(_focusConnection);
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

