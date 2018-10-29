//
//  Keyboard.cpp
//  interface/src/scripting
//
//  Created by Dante Ruiz on 2018-08-27.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Keyboard.h"

#include <utility>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <QtGui/qevent.h>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

#include <PointerEvent.h>
#include <PointerManager.h>
#include <Pick.h>
#include <controllers/UserInputMapper.h>
#include <PathUtils.h>
#include <ResourceManager.h>
#include <SoundCache.h>
#include <AudioInjector.h>
#include <RegisteredMetaTypes.h>
#include <ui/TabletScriptingInterface.h>

#include "ui/overlays/Overlays.h"
#include "ui/overlays/Overlay.h"
#include "ui/overlays/ModelOverlay.h"
#include "ui/overlays/Cube3DOverlay.h"
#include "ui/overlays/Text3DOverlay.h"
#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "avatar/AvatarManager.h"
#include "raypick/PickScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/WindowScriptingInterface.h"
#include "DependencyManager.h"

#include "raypick/StylusPointer.h"
#include "GLMHelpers.h"
#include "Application.h"

static const int LEFT_HAND_CONTROLLER_INDEX = 0;
static const int RIGHT_HAND_CONTROLLER_INDEX = 1;

static const float MALLET_LENGTH = 0.4f;
static const float MALLET_TOUCH_Y_OFFSET = 0.105f;
static const float MALLET_Y_OFFSET = 0.35f;

static const glm::quat MALLET_ROTATION_OFFSET{0.70710678f, 0.0f, -0.70710678f, 0.0f};
static const glm::vec3 MALLET_MODEL_DIMENSIONS{0.05f, MALLET_LENGTH, 0.05f};
static const glm::vec3 MALLET_POSITION_OFFSET{0.0f, -MALLET_Y_OFFSET / 2.0f, 0.0f};
static const glm::vec3 MALLET_TIP_OFFSET{0.0f, MALLET_LENGTH - MALLET_TOUCH_Y_OFFSET, 0.0f};


static const glm::vec3 Z_AXIS {0.0f, 0.0f, 1.0f};
static const glm::vec3 KEYBOARD_TABLET_OFFSET{0.28f, -0.3f, -0.05f};
static const glm::vec3 KEYBOARD_TABLET_DEGREES_OFFSET{-45.0f, 0.0f, 0.0f};
static const glm::vec3 KEYBOARD_TABLET_LANDSCAPE_OFFSET{-0.2f, -0.27f, -0.05f};
static const glm::vec3 KEYBOARD_TABLET_LANDSCAPE_DEGREES_OFFSET{-45.0f, 0.0f, -90.0f};
static const glm::vec3 KEYBOARD_AVATAR_OFFSET{-0.6f, 0.3f, -0.7f};
static const glm::vec3 KEYBOARD_AVATAR_DEGREES_OFFSET{0.0f, 180.0f, 0.0f};

static const QString SOUND_FILE = PathUtils::resourcesUrl() + "sounds/keyboard_key.mp3";
static const QString MALLET_MODEL_URL = PathUtils::resourcesUrl() + "meshes/drumstick.fbx";

static const float PULSE_STRENGTH = 0.6f;
static const float PULSE_DURATION = 3.0f;

static const int KEY_PRESS_TIMEOUT_MS = 100;
static const int LAYER_SWITCH_TIMEOUT_MS = 200;

static const QString CHARACTER_STRING = "character";
static const QString CAPS_STRING = "caps";
static const QString CLOSE_STRING = "close";
static const QString LAYER_STRING = "layer";
static const QString BACKSPACE_STRING = "backspace";
static const QString SPACE_STRING = "space";
static const QString ENTER_STRING = "enter";

std::pair<glm::vec3, glm::quat> calculateKeyboardPositionAndOrientation() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto hmd = DependencyManager::get<HMDScriptingInterface>();

    std::pair<glm::vec3, glm::quat> keyboardLocation = std::make_pair(glm::vec3(), glm::quat());
    float sensorToWorldScale = myAvatar->getSensorToWorldScale();
    QUuid tabletID = hmd->getCurrentTabletFrameID();
    if (!tabletID.isNull() && hmd->getShouldShowTablet()) {
        Overlays& overlays = qApp->getOverlays();
        auto tabletOverlay = std::dynamic_pointer_cast<Base3DOverlay>(overlays.getOverlay(tabletID));
        if (tabletOverlay) {
            auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
            bool landscapeMode = tablet->getLandscape();
            glm::vec3 keyboardOffset = landscapeMode ? KEYBOARD_TABLET_LANDSCAPE_OFFSET : KEYBOARD_TABLET_OFFSET;
            glm::vec3 keyboardDegreesOffset = landscapeMode ? KEYBOARD_TABLET_LANDSCAPE_DEGREES_OFFSET : KEYBOARD_TABLET_DEGREES_OFFSET;
            glm::vec3 tabletWorldPosition = tabletOverlay->getWorldPosition();
            glm::quat tabletWorldOrientation = tabletOverlay->getWorldOrientation();
            glm::vec3 scaledKeyboardTabletOffset = keyboardOffset * sensorToWorldScale;

            keyboardLocation.first = tabletWorldPosition + (tabletWorldOrientation * scaledKeyboardTabletOffset);
            keyboardLocation.second = tabletWorldOrientation * glm::quat(glm::radians(keyboardDegreesOffset));
        }

    } else {
        glm::vec3 avatarWorldPosition = myAvatar->getWorldPosition();
        glm::quat avatarWorldOrientation = myAvatar->getWorldOrientation();
        glm::vec3 scaledKeyboardAvatarOffset = KEYBOARD_AVATAR_OFFSET * sensorToWorldScale;

        keyboardLocation.first = avatarWorldPosition + (avatarWorldOrientation * scaledKeyboardAvatarOffset);
        keyboardLocation.second = avatarWorldOrientation * glm::quat(glm::radians(KEYBOARD_AVATAR_DEGREES_OFFSET));
    }

    return keyboardLocation;
}

void Key::saveDimensionsAndLocalPosition() {
    Overlays& overlays = qApp->getOverlays();
    auto model3DOverlay = std::dynamic_pointer_cast<ModelOverlay>(overlays.getOverlay(_keyID));

    if (model3DOverlay) {
        _originalLocalPosition = model3DOverlay->getLocalPosition();
        _originalDimensions = model3DOverlay->getDimensions();
        _currentLocalPosition = _originalLocalPosition;
    }
}

void Key::scaleKey(float sensorToWorldScale) {
    Overlays& overlays = qApp->getOverlays();
    auto model3DOverlay = std::dynamic_pointer_cast<ModelOverlay>(overlays.getOverlay(_keyID));

    if (model3DOverlay) {
        glm::vec3 scaledLocalPosition = _originalLocalPosition * sensorToWorldScale;
        glm::vec3 scaledDimensions = _originalDimensions * sensorToWorldScale;
        _currentLocalPosition = scaledLocalPosition;

        QVariantMap properties {
            { "dimensions", vec3toVariant(scaledDimensions) },
            { "localPosition", vec3toVariant(scaledLocalPosition) }
        };

        overlays.editOverlay(_keyID, properties);
    }
}

void Key::startTimer(int time) {
    if (_timer) {
        _timer->start(time);
        _timer->setSingleShot(true);
    }
}

bool Key::timerFinished() {
    if (_timer) {
        return (_timer->remainingTime() <= 0);
    }
    return false;
}

QString Key::getKeyString(bool toUpper) const {
    return toUpper ? _keyString.toUpper() : _keyString;
}

int Key::getScanCode(bool toUpper) const {
    QString character = toUpper ? _keyString.toUpper() : _keyString;
    auto utf8Key = character.toUtf8();
    return (int)utf8Key[0];
}

Key::Type Key::getKeyTypeFromString(const QString& keyTypeString) {
    if (keyTypeString == SPACE_STRING) {
        return Type::SPACE;
    } else if (keyTypeString == BACKSPACE_STRING) {
        return Type::BACKSPACE;
    } else if (keyTypeString == LAYER_STRING) {
        return Type::LAYER;
    } else if (keyTypeString == CAPS_STRING) {
        return Type::CAPS;
    } else if (keyTypeString == CLOSE_STRING) {
        return Type::CLOSE;
    } else if (keyTypeString == ENTER_STRING) {
        return Type::ENTER;
    }

    return Type::CHARACTER;
}

Keyboard::Keyboard() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    auto windowScriptingInterface = DependencyManager::get<WindowScriptingInterface>();
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    connect(pointerManager.data(), &PointerManager::triggerBeginOverlay, this, &Keyboard::handleTriggerBegin, Qt::QueuedConnection);
    connect(pointerManager.data(), &PointerManager::triggerContinueOverlay, this, &Keyboard::handleTriggerContinue, Qt::QueuedConnection);
    connect(pointerManager.data(), &PointerManager::triggerEndOverlay, this, &Keyboard::handleTriggerEnd, Qt::QueuedConnection);
    connect(myAvatar.get(), &MyAvatar::sensorToWorldScaleChanged, this, &Keyboard::scaleKeyboard, Qt::QueuedConnection);
    connect(windowScriptingInterface.data(), &WindowScriptingInterface::domainChanged, [&]() { setRaised(false); });
}


void Keyboard::createKeyboard() {
    auto pointerManager = DependencyManager::get<PointerManager>();

    QVariantMap modelProperties {
        { "url", MALLET_MODEL_URL }
    };

    QVariantMap leftStylusProperties {
        { "hand", LEFT_HAND_CONTROLLER_INDEX },
        { "filter", PickScriptingInterface::PICK_OVERLAYS() },
        { "model", modelProperties },
        { "tipOffset", vec3toVariant(MALLET_TIP_OFFSET) }
    };

    QVariantMap rightStylusProperties {
        { "hand", RIGHT_HAND_CONTROLLER_INDEX },
        { "filter", PickScriptingInterface::PICK_OVERLAYS() },
        { "model", modelProperties },
        { "tipOffset", vec3toVariant(MALLET_TIP_OFFSET) }
    };

    _leftHandStylus = pointerManager->addPointer(std::make_shared<StylusPointer>(leftStylusProperties, StylusPointer::buildStylusOverlay(leftStylusProperties), true, true,
                                                                                 MALLET_POSITION_OFFSET, MALLET_ROTATION_OFFSET, MALLET_MODEL_DIMENSIONS));
    _rightHandStylus = pointerManager->addPointer(std::make_shared<StylusPointer>(rightStylusProperties, StylusPointer::buildStylusOverlay(rightStylusProperties), true, true,
                                                                                  MALLET_POSITION_OFFSET, MALLET_ROTATION_OFFSET, MALLET_MODEL_DIMENSIONS));

    pointerManager->disablePointer(_rightHandStylus);
    pointerManager->disablePointer(_leftHandStylus);

    QString keyboardSvg = PathUtils::resourcesUrl() + "config/keyboard.json";
    loadKeyboardFile(keyboardSvg);

    _keySound = DependencyManager::get<SoundCache>()->getSound(SOUND_FILE);
}

bool Keyboard::isRaised() const {
    return resultWithReadLock<bool>([&] { return _raised; });
}

void Keyboard::setRaised(bool raised) {

    bool isRaised;
    withReadLock([&] { isRaised = _raised; });
    if (isRaised != raised) {
        raiseKeyboardAnchor(raised);
        raiseKeyboard(raised);
        raised ? enableStylus() : disableStylus();
        withWriteLock([&] {
            _raised = raised;
            _layerIndex = 0;
            _capsEnabled = false;
            _typedCharacters.clear();
        });

        updateTextDisplay();
    }
}

void Keyboard::updateTextDisplay() {
    Overlays& overlays = qApp->getOverlays();

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    float sensorToWorldScale = myAvatar->getSensorToWorldScale();
    float textWidth = (float) overlays.textSize(_textDisplay.overlayID, _typedCharacters).width();

    glm::vec3 scaledDimensions = _textDisplay.dimensions;
    scaledDimensions *= sensorToWorldScale;
    float leftMargin = (scaledDimensions.x / 2);
    scaledDimensions.x += textWidth;


    QVariantMap textDisplayProperties {
        { "dimensions", vec3toVariant(scaledDimensions) },
        { "leftMargin", leftMargin },
        { "text", _typedCharacters },
        { "lineHeight", (_textDisplay.lineHeight * sensorToWorldScale) }
    };

    overlays.editOverlay(_textDisplay.overlayID, textDisplayProperties);
}

void Keyboard::raiseKeyboardAnchor(bool raise) const {
    Overlays& overlays = qApp->getOverlays();
    OverlayID anchorOverlayID = _anchor.overlayID;
    auto anchorOverlay = std::dynamic_pointer_cast<Cube3DOverlay>(overlays.getOverlay(anchorOverlayID));
    if (anchorOverlay) {
        std::pair<glm::vec3, glm::quat> keyboardLocation = calculateKeyboardPositionAndOrientation();
        anchorOverlay->setWorldPosition(keyboardLocation.first);
        anchorOverlay->setWorldOrientation(keyboardLocation.second);
        anchorOverlay->setVisible(raise);

        QVariantMap textDisplayProperties {
            { "visible", raise }
        };

        overlays.editOverlay(_textDisplay.overlayID, textDisplayProperties);
    }
}

void Keyboard::scaleKeyboard(float sensorToWorldScale) {
    Overlays& overlays = qApp->getOverlays();

    glm::vec3 scaledDimensions = _anchor.originalDimensions * sensorToWorldScale;
    auto volume3DOverlay = std::dynamic_pointer_cast<Volume3DOverlay>(overlays.getOverlay(_anchor.overlayID));

    if (volume3DOverlay) {
        volume3DOverlay->setDimensions(scaledDimensions);
    }

    for (auto& keyboardLayer: _keyboardLayers) {
        for (auto& key: keyboardLayer) {
            key.scaleKey(sensorToWorldScale);
        }
    }



    glm::vec3 scaledLocalPosition = _textDisplay.localPosition * sensorToWorldScale;
    glm::vec3 textDisplayScaledDimensions = _textDisplay.dimensions * sensorToWorldScale;

    QVariantMap textDisplayProperties {
        { "localPosition", vec3toVariant(scaledLocalPosition) },
        { "dimensions", vec3toVariant(textDisplayScaledDimensions) },
        { "lineHeight", (_textDisplay.lineHeight * sensorToWorldScale) }
    };

    overlays.editOverlay(_textDisplay.overlayID, textDisplayProperties);
}

void Keyboard::startLayerSwitchTimer() {
    if (_layerSwitchTimer) {
        _layerSwitchTimer->start(LAYER_SWITCH_TIMEOUT_MS);
        _layerSwitchTimer->setSingleShot(true);
    }
}

bool Keyboard::isLayerSwitchTimerFinished() {
    if (_layerSwitchTimer) {
        return (_layerSwitchTimer->remainingTime() <= 0);
    }
    return false;
}

void Keyboard::raiseKeyboard(bool raise) const {
    if (_keyboardLayers.empty()) {
        return;
    }
    Overlays& overlays = qApp->getOverlays();
    for (const auto& key: _keyboardLayers[_layerIndex]) {
        auto base3DOverlay = std::dynamic_pointer_cast<Base3DOverlay>(overlays.getOverlay(key.getID()));
        if (base3DOverlay) {
            base3DOverlay->setVisible(raise);
        }
    }
}

bool Keyboard::isPassword() const {
    return resultWithReadLock<bool>([&] { return _password; });
}

void Keyboard::setPassword(bool password) {
    if (_password != password) {
        withWriteLock([&] {
            _password = password;
            _typedCharacters.clear();
        });
    }

    updateTextDisplay();
}

void Keyboard::switchToLayer(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < (int)_keyboardLayers.size()) {
        Overlays& overlays = qApp->getOverlays();

        OverlayID currentAnchorOverlayID = _anchor.overlayID;

        glm::vec3 currentOverlayPosition;
        glm::quat currentOverlayOrientation;

        auto currentAnchorOverlay = std::dynamic_pointer_cast<Cube3DOverlay>(overlays.getOverlay(currentAnchorOverlayID));
        if (currentAnchorOverlay) {
            currentOverlayPosition = currentAnchorOverlay->getWorldPosition();
            currentOverlayOrientation = currentAnchorOverlay->getWorldOrientation();
        }

         raiseKeyboardAnchor(false);
         raiseKeyboard(false);

         setLayerIndex(layerIndex);

         raiseKeyboardAnchor(true);
         raiseKeyboard(true);

         OverlayID newAnchorOverlayID = _anchor.overlayID;
         auto newAnchorOverlay = std::dynamic_pointer_cast<Cube3DOverlay>(overlays.getOverlay(newAnchorOverlayID));
         if (newAnchorOverlay) {
             newAnchorOverlay->setWorldPosition(currentOverlayPosition);
             newAnchorOverlay->setWorldOrientation(currentOverlayOrientation);
         }

         startLayerSwitchTimer();
    }
}

void Keyboard::handleTriggerBegin(const OverlayID& overlayID, const PointerEvent& event) {
    if (_keyboardLayers.empty() || !isLayerSwitchTimerFinished()) {
        return;
    }

    auto pointerID = event.getID();
    auto buttonType = event.getButton();

    for (auto index = _keyboardLayers[_layerIndex].begin(); index != _keyboardLayers[_layerIndex].end(); index++) {
        Key& key = *index;
        if (key.getID() == overlayID && (pointerID == _leftHandStylus || pointerID == _rightHandStylus) &&
            buttonType == PointerEvent::PrimaryButton) {


            if (key.timerFinished()) {

                auto handIndex = (pointerID == _leftHandStylus) ? controller::Hand::LEFT : controller::Hand::RIGHT;
                auto userInputMapper = DependencyManager::get<UserInputMapper>();
                userInputMapper->triggerHapticPulse(PULSE_STRENGTH, PULSE_DURATION, handIndex);

                Overlays& overlays = qApp->getOverlays();
                auto base3DOverlay = std::dynamic_pointer_cast<Base3DOverlay>(overlays.getOverlay(overlayID));

                glm::vec3 keyWorldPosition;
                if (base3DOverlay) {
                    keyWorldPosition = base3DOverlay->getWorldPosition();
                }

                AudioInjectorOptions audioOptions;
                audioOptions.localOnly = true;
                audioOptions.position = keyWorldPosition;
                audioOptions.volume = 0.4f;

                AudioInjector::playSound(_keySound->getByteArray(), audioOptions);

                int scanCode = key.getScanCode(_capsEnabled);
                QString keyString = key.getKeyString(_capsEnabled);

                auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");

                switch (key.getKeyType()) {
                    case Key::Type::CLOSE:
                        setRaised(false);
                        tablet->unfocus();
                        return;

                    case Key::Type::CAPS:
                        _capsEnabled = !_capsEnabled;
                        switchToLayer(key.getSwitchToLayerIndex());
                        return;
                    case Key::Type::LAYER:
                        _capsEnabled = false;
                        switchToLayer(key.getSwitchToLayerIndex());
                        return;
                    case Key::Type::BACKSPACE:
                        scanCode = Qt::Key_Backspace;
                        keyString = "\x08";
                        _typedCharacters = _typedCharacters.left(_typedCharacters.length() -1);
                        updateTextDisplay();
                        break;
                    case Key::Type::ENTER:
                        scanCode = Qt::Key_Return;
                        keyString = "\x0d";
                        _typedCharacters.clear();
                        updateTextDisplay();
                        break;
                    case Key::Type::CHARACTER:
                        if (keyString != " ") {
                            _typedCharacters.push_back((_password ? "*" : keyString));
                        } else {
                            _typedCharacters.clear();
                        }
                        updateTextDisplay();
                        break;

                    default:
                        break;
                }

                QKeyEvent* pressEvent = new QKeyEvent(QEvent::KeyPress, scanCode, Qt::NoModifier, keyString);
                QKeyEvent* releaseEvent = new QKeyEvent(QEvent::KeyRelease, scanCode, Qt::NoModifier, keyString);
                QCoreApplication::postEvent(QCoreApplication::instance(), pressEvent);
                QCoreApplication::postEvent(QCoreApplication::instance(), releaseEvent);

                key.startTimer(KEY_PRESS_TIMEOUT_MS);
            }

            break;
        }
    }
}

void Keyboard::handleTriggerEnd(const OverlayID& overlayID, const PointerEvent& event) {
    if (_keyboardLayers.empty() || !isLayerSwitchTimerFinished()) {
        return;
    }

    auto pointerID = event.getID();

    for (auto index = _keyboardLayers[_layerIndex].begin(); index != _keyboardLayers[_layerIndex].end(); index++) {
        Key& key = *index;

        if (key.getID() == overlayID  && (pointerID == _leftHandStylus || pointerID == _rightHandStylus)) {
            Overlays& overlays = qApp->getOverlays();

            auto base3DOverlay = std::dynamic_pointer_cast<Base3DOverlay>(overlays.getOverlay(overlayID));

            if (base3DOverlay) {
                base3DOverlay->setLocalPosition(key.getCurrentLocalPosition());
            }

            key.setIsPressed(false);
            if (key.timerFinished()) {
                key.startTimer(KEY_PRESS_TIMEOUT_MS);
            }
            break;
        }
    }

}

void Keyboard::handleTriggerContinue(const OverlayID& overlayID, const PointerEvent& event) {
    if (_keyboardLayers.empty() || !isLayerSwitchTimerFinished()) {
        return;
    }

    auto pointerID = event.getID();

    for (auto index = _keyboardLayers[_layerIndex].begin(); index != _keyboardLayers[_layerIndex].end(); index++) {
        Key& key = *index;

        if (key.getID() == overlayID && (pointerID == _leftHandStylus || pointerID == _rightHandStylus)) {
            Overlays& overlays = qApp->getOverlays();

            if (!key.isPressed()) {
                auto pointerManager = DependencyManager::get<PointerManager>();
                auto pickResult = pointerManager->getPrevPickResult(pointerID);

                auto base3DOverlay = std::dynamic_pointer_cast<Base3DOverlay>(overlays.getOverlay(overlayID));

                if (base3DOverlay) {
                    auto pickResultVariant = pickResult->toVariantMap();
                    auto stylusTipVariant = pickResultVariant["stylusTip"];
                    auto stylusTipPositionVariant = stylusTipVariant.toMap()["position"];
                    glm::vec3 stylusTipPosition = vec3FromVariant(stylusTipPositionVariant);

                    glm::quat overlayOrientation = base3DOverlay->getWorldOrientation();
                    glm::vec3 overlayPosition = base3DOverlay->getWorldPosition();

                    glm::mat4 overlayWorldMat = createMatFromQuatAndPos(overlayOrientation, overlayPosition);
                    glm::mat4 overlayWorldToLocalMat = glm::inverse(overlayWorldMat);

                    glm::vec3 stylusTipInOverlaySpace = transformPoint(overlayWorldToLocalMat, stylusTipPosition);

                    static const float PENATRATION_THRESHOLD = 0.025f;
                    if (stylusTipInOverlaySpace.z < PENATRATION_THRESHOLD) {
                        static const float Z_OFFSET = 0.002f;
                        glm::vec3 overlayYAxis = overlayOrientation * Z_AXIS;
                        glm::vec3 overlayYOffset = overlayYAxis * Z_OFFSET;
                        glm::vec3 localPosition = key.getCurrentLocalPosition() - overlayYOffset;
                        base3DOverlay->setLocalPosition(localPosition);
                        key.setIsPressed(true);
                    }
                }
            }
            break;
        }
    }
}

void Keyboard::disableStylus() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    pointerManager->setRenderState(_leftHandStylus, "events off");
    pointerManager->disablePointer(_leftHandStylus);
    pointerManager->setRenderState(_rightHandStylus, "events off");
    pointerManager->disablePointer(_rightHandStylus);
}

void Keyboard::setLayerIndex(int layerIndex) {
    if (layerIndex >= 0 && layerIndex < (int)_keyboardLayers.size()) {
        _layerIndex = layerIndex;
    } else {
        _layerIndex = 0;
    }
}

void Keyboard::loadKeyboardFile(const QString& keyboardFile) {
    if (keyboardFile.isEmpty()) {
        return;
    }

    auto request = DependencyManager::get<ResourceManager>()->createResourceRequest(this, keyboardFile);

    if (!request) {
        qCWarning(interfaceapp) << "Could not create resource for Keyboard file" << keyboardFile;
    }


    connect(request, &ResourceRequest::finished, this, [=]() {
        if (request->getResult() != ResourceRequest::Success) {
            qCWarning(interfaceapp) << "Keyboard file failed to download";
            return;
        }

        clearKeyboardKeys();
        Overlays& overlays = qApp->getOverlays();
        auto requestData = request->getData();

        QVector<QUuid> includeItems;

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(requestData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(interfaceapp) << "Failed to parse keyboard json file - Error: " << parseError.errorString();
            return;
        }
        QJsonObject jsonObject = jsonDoc.object();
        QJsonArray layer = jsonObject["Layer1"].toArray();
        QJsonObject anchorObject = jsonObject["anchor"].toObject();
        bool useResourcePath = jsonObject["useResourcesPath"].toBool();
        QString resourcePath = PathUtils::resourcesUrl();


        if (anchorObject.isEmpty()) {
            qCWarning(interfaceapp) << "No Anchor specified. Not creating keyboard";
            return;
        }

        QVariantMap anchorProperties {
            { "name", "KeyboardAnchor"},
            { "isSolid", true },
            { "visible", false },
            { "grabbable", true },
            { "ignoreRayIntersection", false },
            { "dimensions", anchorObject["dimensions"].toVariant() },
            { "position", anchorObject["position"].toVariant() },
            { "orientation", anchorObject["rotation"].toVariant() }
        };

        glm::vec3 dimensions = vec3FromVariant(anchorObject["dimensions"].toVariant());

        Anchor anchor;
        anchor.overlayID = overlays.addOverlay("cube", anchorProperties);
        anchor.originalDimensions = dimensions;
        _anchor = anchor;

        const QJsonArray& keyboardLayers = jsonObject["layers"].toArray();
        int keyboardLayerCount = keyboardLayers.size();
        _keyboardLayers.reserve(keyboardLayerCount);


        for (int keyboardLayerIndex = 0; keyboardLayerIndex < keyboardLayerCount; keyboardLayerIndex++) {
            const QJsonValue& keyboardLayer = keyboardLayers[keyboardLayerIndex].toArray();

            std::vector<Key> keyboardLayerKeys;
            foreach (const QJsonValue& keyboardKeyValue, keyboardLayer.toArray()) {

                QVariantMap textureMap;
                if (!keyboardKeyValue["texture"].isNull()) {
                    textureMap = keyboardKeyValue["texture"].toObject().toVariantMap();

                    if (useResourcePath) {
                        for (auto iter = textureMap.begin(); iter != textureMap.end(); iter++) {
                            QString modifiedPath = resourcePath + iter.value().toString();
                            textureMap[iter.key()] = modifiedPath;
                        }
                    }
                }

                QString modelUrl = keyboardKeyValue["modelURL"].toString();
                QString url = (useResourcePath ? (resourcePath + modelUrl) : modelUrl);

                QVariantMap properties {
                    { "dimensions", keyboardKeyValue["dimensions"].toVariant() },
                    { "position", keyboardKeyValue["position"].toVariant() },
                    { "visible", false },
                    { "isSolid", true },
                    { "emissive", true },
                    { "parentID", _anchor.overlayID },
                    { "url", url },
                    { "textures", textureMap },
                    { "grabbable", false },
                    { "localOrientation", keyboardKeyValue["localOrientation"].toVariant() }
                };

                OverlayID overlayID = overlays.addOverlay("model", properties);

                QString keyType = keyboardKeyValue["type"].toString();
                QString keyString = keyboardKeyValue["key"].toString();

                Key key;
                if (!keyType.isNull()) {
                    Key::Type type= Key::getKeyTypeFromString(keyType);
                    key.setKeyType(type);

                    if (type == Key::Type::LAYER || type == Key::Type::CAPS) {
                        int switchToLayer = keyboardKeyValue["switchToLayer"].toInt();
                        key.setSwitchToLayerIndex(switchToLayer);
                    }
                }
                key.setID(overlayID);
                key.setKeyString(keyString);
                key.saveDimensionsAndLocalPosition();

                includeItems.append(key.getID());
                _itemsToIgnore.append(key.getID());
                keyboardLayerKeys.push_back(key);
            }

            _keyboardLayers.push_back(keyboardLayerKeys);

        }

        TextDisplay textDisplay;
        QJsonObject displayTextObject = jsonObject["textDisplay"].toObject();

        QVariantMap displayTextProperties {
            { "dimensions", displayTextObject["dimensions"].toVariant() },
            { "localPosition", displayTextObject["localPosition"].toVariant() },
            { "localOrientation", displayTextObject["localOrientation"].toVariant() },
            { "leftMargin", displayTextObject["leftMargin"].toVariant() },
            { "rightMargin", displayTextObject["rightMargin"].toVariant() },
            { "topMargin", displayTextObject["topMargin"].toVariant() },
            { "bottomMargin", displayTextObject["bottomMargin"].toVariant() },
            { "lineHeight", displayTextObject["lineHeight"].toVariant() },
            { "visible", false },
            { "emissive", true },
            { "grabbable", false },
            { "text", ""},
            { "parentID", _anchor.overlayID }
        };

        textDisplay.overlayID = overlays.addOverlay("text3d", displayTextProperties);
        textDisplay.localPosition = vec3FromVariant(displayTextObject["localPosition"].toVariant());
        textDisplay.dimensions = vec3FromVariant(displayTextObject["dimensions"].toVariant());
        textDisplay.lineHeight = (float) displayTextObject["lineHeight"].toDouble();

        _textDisplay = textDisplay;

        _ignoreItemsLock.withWriteLock([&] {
            _itemsToIgnore.push_back(_textDisplay.overlayID);
            _itemsToIgnore.push_back(_anchor.overlayID);
        });
        _layerIndex = 0;
        auto pointerManager = DependencyManager::get<PointerManager>();
        pointerManager->setIncludeItems(_leftHandStylus, includeItems);
        pointerManager->setIncludeItems(_rightHandStylus, includeItems);
    });

    request->send();
}

QVector<OverlayID> Keyboard::getKeysID() {
    return _ignoreItemsLock.resultWithReadLock<QVector<OverlayID>>([&] {
        return _itemsToIgnore;
    });
}

void Keyboard::clearKeyboardKeys() {
    Overlays& overlays = qApp->getOverlays();

    for (const auto& keyboardLayer: _keyboardLayers) {
        for (const Key& key: keyboardLayer) {
            overlays.deleteOverlay(key.getID());
        }
    }

    overlays.deleteOverlay(_anchor.overlayID);
    overlays.deleteOverlay(_textDisplay.overlayID);

    _keyboardLayers.clear();

    _ignoreItemsLock.withWriteLock([&] {
        _itemsToIgnore.clear();
    });
}

void Keyboard::enableStylus() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    pointerManager->setRenderState(_leftHandStylus, "events on");
    pointerManager->enablePointer(_leftHandStylus);
    pointerManager->setRenderState(_rightHandStylus, "events on");
    pointerManager->enablePointer(_rightHandStylus);

}
