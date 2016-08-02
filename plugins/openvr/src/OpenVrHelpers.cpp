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
#include <controllers/Pose.h>
#include <NumericalConstants.h>
#include <ui-plugins/PluginContainer.h>
#include <ui/Menu.h>
#include "../../interface/src/Menu.h"

Q_DECLARE_LOGGING_CATEGORY(displayplugins)
Q_LOGGING_CATEGORY(displayplugins, "hifi.plugins.display")

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

static int refCount { 0 };
static Mutex mutex;
static vr::IVRSystem* activeHmd { nullptr };

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
            #if DEV_BUILD
                qCDebug(displayplugins) << "OpenVR: No vr::IVRSystem instance active, building";
            #endif
            vr::EVRInitError eError = vr::VRInitError_None;
            activeHmd = vr::VR_Init(&eError, vr::VRApplication_Scene);
            #if DEV_BUILD
                qCDebug(displayplugins) << "OpenVR display: HMD is " << activeHmd << " error is " << eError;
            #endif
        }
        if (activeHmd) {
            #if DEV_BUILD
                qCDebug(displayplugins) << "OpenVR: incrementing refcount";
            #endif
            ++refCount;
        }
    } else {
        #if DEV_BUILD
            qCDebug(displayplugins) << "OpenVR: no hmd present";
        #endif
    }
    return activeHmd;
}

void releaseOpenVrSystem() {
    if (activeHmd) {
        Lock lock(mutex);
        #if DEV_BUILD
            qCDebug(displayplugins) << "OpenVR: decrementing refcount";
        #endif
        --refCount;
        if (0 == refCount) {
            #if DEV_BUILD
                qCDebug(displayplugins) << "OpenVR: zero refcount, deallocate VR system";
            #endif
            vr::VR_Shutdown();
            activeHmd = nullptr;
        }
    }
}

static char textArray[8192];

static QMetaObject::Connection _focusConnection, _focusTextConnection, _overlayMenuConnection;
extern bool _openVrDisplayActive;
static vr::IVROverlay* _overlay { nullptr };
static QObject* _keyboardFocusObject { nullptr };
static QString _existingText;
static Qt::InputMethodHints _currentHints;
extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
static bool _keyboardShown { false };
static bool _overlayRevealed { false };
static const uint32_t SHOW_KEYBOARD_DELAY_MS = 400;

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
                mat4 headPose = cancelOutRollAndPitch(toGlm(_trackedDevicePose[0].mDeviceToAbsoluteTracking));
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

void updateFromOpenVrKeyboardInput() {
    auto chars = _overlay->GetKeyboardText(textArray, 8192);
    auto newText = QString(QByteArray(textArray, chars));
    _keyboardFocusObject->setProperty("text", newText);
    //// TODO modify the new text to match the possible input hints:
    //// ImhDigitsOnly  ImhFormattedNumbersOnly  ImhUppercaseOnly  ImhLowercaseOnly 
    //// ImhDialableCharactersOnly ImhEmailCharactersOnly  ImhUrlCharactersOnly  ImhLatinOnly
    //QInputMethodEvent event(_existingText, QList<QInputMethodEvent::Attribute>());
    //event.setCommitString(newText, 0, _existingText.size());
    //qApp->sendEvent(_keyboardFocusObject, &event);
}
void finishOpenVrKeyboardInput() {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    updateFromOpenVrKeyboardInput();
    // Simulate an enter press on the top level window to trigger the action
    if (0 == (_currentHints & Qt::ImhMultiLine)) {
        qApp->sendEvent(offscreenUi->getWindow(), &QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::KeyboardModifiers(), QString("\n")));
        qApp->sendEvent(offscreenUi->getWindow(), &QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::KeyboardModifiers()));
    }
}

static const QString DEBUG_FLAG("HIFI_DISABLE_STEAM_VR_KEYBOARD");
bool disableSteamVrKeyboard = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);

void enableOpenVrKeyboard(PluginContainer* container) {
    if (disableSteamVrKeyboard) {
        return;
    }
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    _overlay = vr::VROverlay();

    
    auto menu = container->getPrimaryMenu();
    auto action = menu->getActionForOption(MenuOption::Overlays);

    // When the overlays are revealed, suppress the keyboard from appearing on text focus for a tenth of a second. 
    _overlayMenuConnection = QObject::connect(action, &QAction::triggered, [action] {
        if (action->isChecked()) {
            _overlayRevealed = true;
            const int KEYBOARD_DELAY_MS = 100;
            QTimer::singleShot(KEYBOARD_DELAY_MS, [&] { _overlayRevealed = false; });
        }
    });

    _focusConnection = QObject::connect(offscreenUi->getWindow(), &QQuickWindow::focusObjectChanged, [](QObject* object) {
        if (object != _keyboardFocusObject) {
            showOpenVrKeyboard(false);
        }
    });

    _focusTextConnection = QObject::connect(offscreenUi.data(), &OffscreenUi::focusTextChanged, [](bool focusText) {
        if (_openVrDisplayActive) {
            if (_overlayRevealed) {
                // suppress at most one text focus event
                _overlayRevealed = false;
                return;
            }
            showOpenVrKeyboard(focusText);
        }
    });
}


void disableOpenVrKeyboard() {
    if (disableSteamVrKeyboard) {
        return;
    }
    QObject::disconnect(_overlayMenuConnection);
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
                activeHmd->AcknowledgeQuit_Exiting();
                QMetaObject::invokeMethod(qApp, "quit");
                break;

            case vr::VREvent_KeyboardCharInput:
                // Make the focused field match the keyboard results, inclusive of combining characters and such.
                updateFromOpenVrKeyboardInput();
                break;

            case vr::VREvent_KeyboardDone: 
                finishOpenVrKeyboardInput();

            // FALL THROUGH
            case vr::VREvent_KeyboardClosed:
                _keyboardFocusObject = nullptr;
                _keyboardShown = false;
                DependencyManager::get<OffscreenUi>()->unfocusWindows();
                break;

            default:
                break;
        }
        #if DEV_BUILD
            qDebug() << "OpenVR: Event " << activeHmd->GetEventTypeNameFromEnum((vr::EVREventType)event.eventType) << "(" << event.eventType << ")";
        #endif
    }

}

controller::Pose openVrControllerPoseToHandPose(bool isLeftHand, const mat4& mat, const vec3& linearVelocity, const vec3& angularVelocity) {
    // When the sensor-to-world rotation is identity the coordinate axes look like this:
    //
    //                       user
    //                      forward
    //                        -z
    //                         |
    //                        y|      user
    //      y                  o----x right
    //       o-----x         user
    //       |                up
    //       |
    //       z
    //
    //     Rift

    // From ABOVE the hand canonical axes looks like this:
    //
    //      | | | |          y        | | | |
    //      | | | |          |        | | | |
    //      |     |          |        |     |
    //      |left | /  x---- +      \ |right|
    //      |     _/          z      \_     |
    //       |   |                     |   |
    //       |   |                     |   |
    //

    // So when the user is in Rift space facing the -zAxis with hands outstretched and palms down
    // the rotation to align the Touch axes with those of the hands is:
    //
    //    touchToHand = halfTurnAboutY * quaterTurnAboutX

    // Due to how the Touch controllers fit into the palm there is an offset that is different for each hand.
    // You can think of this offset as the inverse of the measured rotation when the hands are posed, such that
    // the combination (measurement * offset) is identity at this orientation.
    //
    //    Qoffset = glm::inverse(deltaRotation when hand is posed fingers forward, palm down)
    //
    // An approximate offset for the Touch can be obtained by inspection:
    //
    //    Qoffset = glm::inverse(glm::angleAxis(sign * PI/2.0f, zAxis) * glm::angleAxis(PI/4.0f, xAxis))
    //
    // So the full equation is:
    //
    //    Q = combinedMeasurement * touchToHand
    //
    //    Q = (deltaQ * QOffset) * (yFlip * quarterTurnAboutX)
    //
    //    Q = (deltaQ * inverse(deltaQForAlignedHand)) * (yFlip * quarterTurnAboutX)
    static const glm::quat yFlip = glm::angleAxis(PI, Vectors::UNIT_Y);
    static const glm::quat quarterX = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_X);
    static const glm::quat touchToHand = yFlip * quarterX;

    static const glm::quat leftQuarterZ = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat rightQuarterZ = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat eighthX = glm::angleAxis(PI / 4.0f, Vectors::UNIT_X);

    static const glm::quat leftRotationOffset = glm::inverse(leftQuarterZ * eighthX) * touchToHand;
    static const glm::quat rightRotationOffset = glm::inverse(rightQuarterZ * eighthX) * touchToHand;

    static const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
    static const glm::vec3 CONTROLLER_OFFSET = glm::vec3(CONTROLLER_LENGTH_OFFSET / 2.0f,
        CONTROLLER_LENGTH_OFFSET / 2.0f,
        CONTROLLER_LENGTH_OFFSET * 2.0f);
    static const glm::vec3 leftTranslationOffset = glm::vec3(-1.0f, 1.0f, 1.0f) * CONTROLLER_OFFSET;
    static const glm::vec3 rightTranslationOffset = CONTROLLER_OFFSET;

    auto translationOffset = (isLeftHand ? leftTranslationOffset : rightTranslationOffset);
    auto rotationOffset = (isLeftHand ? leftRotationOffset : rightRotationOffset);

    glm::vec3 position = extractTranslation(mat);
    glm::quat rotation = glm::normalize(glm::quat_cast(mat));

    position += rotation * translationOffset;
    rotation = rotation * rotationOffset;

    // transform into avatar frame
    auto result = controller::Pose(position, rotation);
    // handle change in velocity due to translationOffset
    result.velocity = linearVelocity + glm::cross(angularVelocity, position - extractTranslation(mat));
    result.angularVelocity = angularVelocity;
    return result;
}
