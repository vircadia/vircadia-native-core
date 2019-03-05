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

#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "avatar/AvatarManager.h"
#include "raypick/PickScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/WindowScriptingInterface.h"
#include "scripting/SelectionScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "DependencyManager.h"

#include "raypick/StylusPointer.h"
#include "GLMHelpers.h"
#include "Application.h"

static const int LEFT_HAND_CONTROLLER_INDEX = 0;
static const int RIGHT_HAND_CONTROLLER_INDEX = 1;

static const float MALLET_LENGTH = 0.18f;
static const float MALLET_TOUCH_Y_OFFSET = 0.050f;
static const float MALLET_Y_OFFSET = 0.160f;

static const glm::quat MALLET_ROTATION_OFFSET{0.70710678f, 0.0f, -0.70710678f, 0.0f};
static const glm::vec3 MALLET_MODEL_DIMENSIONS{0.01f, MALLET_LENGTH, 0.01f};
static const glm::vec3 MALLET_POSITION_OFFSET{0.0f, -MALLET_Y_OFFSET / 2.0f, 0.0f};
static const glm::vec3 MALLET_TIP_OFFSET{0.0f, MALLET_LENGTH - MALLET_TOUCH_Y_OFFSET, 0.0f};


static const glm::vec3 Z_AXIS {0.0f, 0.0f, 1.0f};
static const glm::vec3 KEYBOARD_TABLET_OFFSET{0.30f, -0.38f, -0.04f};
static const glm::vec3 KEYBOARD_TABLET_DEGREES_OFFSET{-45.0f, 0.0f, 0.0f};
static const glm::vec3 KEYBOARD_TABLET_LANDSCAPE_OFFSET{-0.2f, -0.27f, -0.05f};
static const glm::vec3 KEYBOARD_TABLET_LANDSCAPE_DEGREES_OFFSET{-45.0f, 0.0f, -90.0f};
static const glm::vec3 KEYBOARD_AVATAR_OFFSET{-0.3f, 0.0f, -0.7f};
static const glm::vec3 KEYBOARD_AVATAR_DEGREES_OFFSET{0.0f, 180.0f, 0.0f};

static const QString SOUND_FILE = PathUtils::resourcesUrl() + "sounds/keyboardPress.mp3";
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

static const QString KEY_HOVER_HIGHLIGHT = "keyHoverHiglight";
static const QString KEY_PRESSED_HIGHLIGHT = "keyPressesHighlight";
static const QVariantMap KEY_HOVERING_STYLE {
    { "isOutlineSmooth", true },
    { "outlineWidth", 3 },
    { "outlineUnoccludedColor", QVariantMap {{"red", 13}, {"green", 152}, {"blue", 186}}},
    { "outlineUnoccludedAlpha", 1.0 },
    { "outlineOccludedAlpha", 0.0 },
    { "fillUnoccludedAlpha", 0.0 },
    { "fillOccludedAlpha", 0.0 }
};

static const QVariantMap KEY_PRESSING_STYLE {
    { "isOutlineSmooth", true },
    { "outlineWidth", 3 },
    { "fillUnoccludedColor", QVariantMap {{"red", 50}, {"green", 50}, {"blue", 50}}},
    { "outlineUnoccludedAlpha", 0.0 },
    { "outlineOccludedAlpha", 0.0 },
    { "fillUnoccludedAlpha", 0.6 },
    { "fillOccludedAlpha", 0.0 }
};

std::pair<glm::vec3, glm::quat> calculateKeyboardPositionAndOrientation() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto hmd = DependencyManager::get<HMDScriptingInterface>();

    std::pair<glm::vec3, glm::quat> keyboardLocation = std::make_pair(glm::vec3(), glm::quat());
    float sensorToWorldScale = myAvatar->getSensorToWorldScale();
    QUuid tabletID = hmd->getCurrentTabletFrameID();
    if (!tabletID.isNull() && hmd->getShouldShowTablet()) {
        EntityPropertyFlags desiredProperties;
        desiredProperties += PROP_POSITION;
        desiredProperties += PROP_ROTATION;
        auto properties = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(tabletID, desiredProperties);

        auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
        bool landscapeMode = tablet->getLandscape();
        glm::vec3 keyboardOffset = landscapeMode ? KEYBOARD_TABLET_LANDSCAPE_OFFSET : KEYBOARD_TABLET_OFFSET;
        glm::vec3 keyboardDegreesOffset = landscapeMode ? KEYBOARD_TABLET_LANDSCAPE_DEGREES_OFFSET : KEYBOARD_TABLET_DEGREES_OFFSET;
        glm::vec3 tabletWorldPosition = properties.getPosition();
        glm::quat tabletWorldOrientation = properties.getRotation();
        glm::vec3 scaledKeyboardTabletOffset = keyboardOffset * sensorToWorldScale;

        keyboardLocation.first = tabletWorldPosition + (tabletWorldOrientation * scaledKeyboardTabletOffset);
        keyboardLocation.second = tabletWorldOrientation * glm::quat(glm::radians(keyboardDegreesOffset));
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
    EntityPropertyFlags desiredProperties;
    desiredProperties += PROP_LOCAL_POSITION;
    desiredProperties += PROP_DIMENSIONS;
    auto properties = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(_keyID, desiredProperties);

    _originalLocalPosition = properties.getLocalPosition();
    _originalDimensions = properties.getDimensions();
    _currentLocalPosition = _originalLocalPosition;
    _originalDimensionsAndLocalPositionSaved = true;
}

void Key::scaleKey(float sensorToWorldScale) {
    if (_originalDimensionsAndLocalPositionSaved) {
        glm::vec3 scaledLocalPosition = _originalLocalPosition * sensorToWorldScale;
        glm::vec3 scaledDimensions = _originalDimensions * sensorToWorldScale;
        _currentLocalPosition = scaledLocalPosition;

        EntityItemProperties properties;
        properties.setDimensions(scaledDimensions);
        properties.setLocalPosition(scaledLocalPosition);
        DependencyManager::get<EntityScriptingInterface>()->editEntity(_keyID, properties);
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
    auto hmdScriptingInterface = DependencyManager::get<HMDScriptingInterface>();
    connect(pointerManager.data(), &PointerManager::triggerBeginOverlay, this, &Keyboard::handleTriggerBegin, Qt::QueuedConnection);
    connect(pointerManager.data(), &PointerManager::triggerContinueOverlay, this, &Keyboard::handleTriggerContinue, Qt::QueuedConnection);
    connect(pointerManager.data(), &PointerManager::triggerEndOverlay, this, &Keyboard::handleTriggerEnd, Qt::QueuedConnection);
    connect(pointerManager.data(), &PointerManager::hoverBeginOverlay, this, &Keyboard::handleHoverBegin, Qt::QueuedConnection);
    connect(pointerManager.data(), &PointerManager::hoverEndOverlay, this, &Keyboard::handleHoverEnd, Qt::QueuedConnection);
    connect(myAvatar.get(), &MyAvatar::sensorToWorldScaleChanged, this, &Keyboard::scaleKeyboard, Qt::QueuedConnection);
    connect(windowScriptingInterface.data(), &WindowScriptingInterface::domainChanged, [&]() { setRaised(false); });
    connect(hmdScriptingInterface.data(), &HMDScriptingInterface::displayModeChanged, [&]() { setRaised(false); });
}

void Keyboard::registerKeyboardHighlighting() {
    auto selection = DependencyManager::get<SelectionScriptingInterface>();
    selection->enableListHighlight(KEY_HOVER_HIGHLIGHT, KEY_HOVERING_STYLE);
    selection->enableListToScene(KEY_HOVER_HIGHLIGHT);
    selection->enableListHighlight(KEY_PRESSED_HIGHLIGHT, KEY_PRESSING_STYLE);
    selection->enableListToScene(KEY_PRESSED_HIGHLIGHT);
}

void Keyboard::disableSelectionLists() {
    auto selection = DependencyManager::get<SelectionScriptingInterface>();
    selection->disableListHighlight(KEY_HOVER_HIGHLIGHT);
    selection->disableListHighlight(KEY_PRESSED_HIGHLIGHT);
}

void Keyboard::enableSelectionLists() {
    auto selection = DependencyManager::get<SelectionScriptingInterface>();
    selection->enableListHighlight(KEY_HOVER_HIGHLIGHT, KEY_HOVERING_STYLE);
    selection->enableListHighlight(KEY_PRESSED_HIGHLIGHT, KEY_PRESSING_STYLE);
}

bool Keyboard::getUse3DKeyboard() const {
    return _use3DKeyboardLock.resultWithReadLock<bool>([&] {
        return _use3DKeyboard.get();
    });
}

void Keyboard::setUse3DKeyboard(bool use) {
    _use3DKeyboardLock.withWriteLock([&] {
        _use3DKeyboard.set(use);
    });
}

void Keyboard::createKeyboard() {
    auto pointerManager = DependencyManager::get<PointerManager>();

    if (_created) {
        pointerManager->removePointer(_leftHandStylus);
        pointerManager->removePointer(_rightHandStylus);
        clearKeyboardKeys();
    }

    QVariantMap modelProperties {
        { "url", MALLET_MODEL_URL }
    };

    QVariantMap leftStylusProperties {
        { "hand", LEFT_HAND_CONTROLLER_INDEX },
        { "filter", PickScriptingInterface::PICK_LOCAL_ENTITIES() },
        { "model", modelProperties },
        { "tipOffset", vec3toVariant(MALLET_TIP_OFFSET) }
    };

    QVariantMap rightStylusProperties {
        { "hand", RIGHT_HAND_CONTROLLER_INDEX },
        { "filter", PickScriptingInterface::PICK_LOCAL_ENTITIES() },
        { "model", modelProperties },
        { "tipOffset", vec3toVariant(MALLET_TIP_OFFSET) }
    };

    _leftHandStylus = pointerManager->addPointer(std::make_shared<StylusPointer>(leftStylusProperties, StylusPointer::buildStylus(leftStylusProperties), true, true,
                                                                                 MALLET_POSITION_OFFSET, MALLET_ROTATION_OFFSET, MALLET_MODEL_DIMENSIONS));
    _rightHandStylus = pointerManager->addPointer(std::make_shared<StylusPointer>(rightStylusProperties, StylusPointer::buildStylus(rightStylusProperties), true, true,
                                                                                  MALLET_POSITION_OFFSET, MALLET_ROTATION_OFFSET, MALLET_MODEL_DIMENSIONS));

    pointerManager->disablePointer(_rightHandStylus);
    pointerManager->disablePointer(_leftHandStylus);

    QString keyboardSvg = PathUtils::resourcesUrl() + "config/keyboard.json";
    loadKeyboardFile(keyboardSvg);

    _keySound = DependencyManager::get<SoundCache>()->getSound(SOUND_FILE);

    _created = true;
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
        raised ? enableSelectionLists() : disableSelectionLists();
        withWriteLock([&] {
            _raised = raised;
            _layerIndex = 0;
            _capsEnabled = false;
            _typedCharacters.clear();
            addIncludeItemsToMallets();
        });

        updateTextDisplay();
    }
}

void Keyboard::addIncludeItemsToMallets() {
    if (_layerIndex >= 0 && _layerIndex < (int)_keyboardLayers.size()) {
        QVector<QUuid> includeItems = _keyboardLayers[_layerIndex].keys().toVector();
        auto pointerManager = DependencyManager::get<PointerManager>();
        pointerManager->setIncludeItems(_leftHandStylus, includeItems);
        pointerManager->setIncludeItems(_rightHandStylus, includeItems);
    }
}

void Keyboard::updateTextDisplay() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    float sensorToWorldScale = myAvatar->getSensorToWorldScale();
    float textWidth = (float)entityScriptingInterface->textSize(_textDisplay.entityID, _typedCharacters).width();

    glm::vec3 scaledDimensions = _textDisplay.dimensions;
    scaledDimensions *= sensorToWorldScale;
    float leftMargin = (scaledDimensions.x / 2);
    scaledDimensions.x += textWidth;
    scaledDimensions.y *= 1.25f;

    EntityItemProperties properties;
    properties.setDimensions(scaledDimensions);
    properties.setLeftMargin(leftMargin);
    properties.setText(_typedCharacters);
    properties.setLineHeight(_textDisplay.lineHeight * sensorToWorldScale);
    entityScriptingInterface->editEntity(_textDisplay.entityID, properties);
}

void Keyboard::raiseKeyboardAnchor(bool raise) const {
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    EntityItemProperties properties;
    properties.setVisible(raise);

    entityScriptingInterface->editEntity(_textDisplay.entityID, properties);
    entityScriptingInterface->editEntity(_backPlate.entityID, properties);

    if (_resetKeyboardPositionOnRaise) {
        std::pair<glm::vec3, glm::quat> keyboardLocation = calculateKeyboardPositionAndOrientation();
        properties.setPosition(keyboardLocation.first);
        properties.setRotation(keyboardLocation.second);
    }
    entityScriptingInterface->editEntity(_anchor.entityID, properties);
}

void Keyboard::scaleKeyboard(float sensorToWorldScale) {
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    {
        EntityItemProperties properties;
        properties.setLocalPosition(_backPlate.localPosition * sensorToWorldScale);
        properties.setDimensions(_backPlate.dimensions * sensorToWorldScale);
        entityScriptingInterface->editEntity(_backPlate.entityID, properties);
    }

    for (auto& keyboardLayer : _keyboardLayers) {
        for (auto iter = keyboardLayer.begin(); iter != keyboardLayer.end(); iter++) {
            iter.value().scaleKey(sensorToWorldScale);
        }
    }

    {
        EntityItemProperties properties;
        properties.setLocalPosition(_textDisplay.localPosition * sensorToWorldScale);
        properties.setDimensions(_textDisplay.dimensions * sensorToWorldScale);
        properties.setLineHeight(_textDisplay.lineHeight * sensorToWorldScale);
        entityScriptingInterface->editEntity(_textDisplay.entityID, properties);
    }
}

void Keyboard::startLayerSwitchTimer() {
    if (_layerSwitchTimer) {
        _layerSwitchTimer->start(LAYER_SWITCH_TIMEOUT_MS);
        _layerSwitchTimer->setSingleShot(true);
    }
}

bool Keyboard::isLayerSwitchTimerFinished() const {
    if (_layerSwitchTimer) {
        return (_layerSwitchTimer->remainingTime() <= 0);
    }
    return false;
}

void Keyboard::raiseKeyboard(bool raise) const {
    if (_keyboardLayers.empty()) {
        return;
    }

    const auto& keyboardLayer = _keyboardLayers[_layerIndex];
    EntityItemProperties properties;
    properties.setVisible(raise);
    for (auto iter = keyboardLayer.begin(); iter != keyboardLayer.end(); iter++) {
        DependencyManager::get<EntityScriptingInterface>()->editEntity(iter.key(), properties);
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

void Keyboard::setResetKeyboardPositionOnRaise(bool reset) {
    if (_resetKeyboardPositionOnRaise != reset) {
        withWriteLock([&] {
            _resetKeyboardPositionOnRaise = reset;
        });
    }
}

void Keyboard::setPreferMalletsOverLasers(bool preferMalletsOverLasers) {
    _preferMalletsOverLasersSettingLock.withWriteLock([&] {
        _preferMalletsOverLasers.set(preferMalletsOverLasers);
    });
}

bool Keyboard::getPreferMalletsOverLasers() const {
    return _preferMalletsOverLasersSettingLock.resultWithReadLock<bool>([&] {
        return _preferMalletsOverLasers.get();
    });
}

void Keyboard::switchToLayer(int layerIndex) {
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    if (layerIndex >= 0 && layerIndex < (int)_keyboardLayers.size()) {
        EntityPropertyFlags desiredProperties;
        desiredProperties += PROP_POSITION;
        desiredProperties += PROP_ROTATION;
        auto oldProperties = entityScriptingInterface->getEntityProperties(_anchor.entityID, desiredProperties);

        glm::vec3 currentPosition = oldProperties.getPosition();
        glm::quat currentOrientation = oldProperties.getRotation();

         raiseKeyboardAnchor(false);
         raiseKeyboard(false);

         setLayerIndex(layerIndex);

         raiseKeyboardAnchor(true);
         raiseKeyboard(true);

         EntityItemProperties properties;
         properties.setPosition(currentPosition);
         properties.setRotation(currentOrientation);
         entityScriptingInterface->editEntity(_anchor.entityID, properties);

         addIncludeItemsToMallets();

         startLayerSwitchTimer();
    }
}

bool Keyboard::shouldProcessEntityAndPointerEvent(const PointerEvent& event) const {
    return (shouldProcessPointerEvent(event) && shouldProcessEntity());
}

bool Keyboard::shouldProcessPointerEvent(const PointerEvent& event) const {
    bool preferMalletsOverLasers = getPreferMalletsOverLasers();
    unsigned int pointerID = event.getID();
    bool isStylusEvent = (pointerID == _leftHandStylus || pointerID == _rightHandStylus);
    bool isLaserEvent = (pointerID == _leftHandLaser || pointerID == _rightHandLaser);
    return ((isStylusEvent && preferMalletsOverLasers) || (isLaserEvent && !preferMalletsOverLasers));
}

void Keyboard::handleTriggerBegin(const QUuid& id, const PointerEvent& event) {
    auto buttonType = event.getButton();
    if (!shouldProcessEntityAndPointerEvent(event) || buttonType != PointerEvent::PrimaryButton) {
        return;
    }

    auto& keyboardLayer = _keyboardLayers[_layerIndex];
    auto search = keyboardLayer.find(id);

    if (search == keyboardLayer.end()) {
        return;
    }

    Key& key = search.value();

    if (key.timerFinished()) {
        unsigned int pointerID = event.getID();
        auto handIndex = (pointerID == _leftHandStylus || pointerID == _leftHandLaser)
            ? controller::Hand::LEFT : controller::Hand::RIGHT;

        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->triggerHapticPulse(PULSE_STRENGTH, PULSE_DURATION, handIndex);

        EntityPropertyFlags desiredProperties;
        desiredProperties += PROP_POSITION;
        glm::vec3 keyWorldPosition = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(id, desiredProperties).getPosition();

        AudioInjectorOptions audioOptions;
        audioOptions.localOnly = true;
        audioOptions.position = keyWorldPosition;
        audioOptions.volume = 0.05f;

        AudioInjector::playSoundAndDelete(_keySound, audioOptions);

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

        if (!getPreferMalletsOverLasers()) {
            key.startTimer(KEY_PRESS_TIMEOUT_MS);
        }
        auto selection = DependencyManager::get<SelectionScriptingInterface>();
        selection->addToSelectedItemsList(KEY_PRESSED_HIGHLIGHT, "entity", id);
    }
}

void Keyboard::setLeftHandLaser(unsigned int leftHandLaser) {
    _handLaserLock.withWriteLock([&] {
        _leftHandLaser = leftHandLaser;
    });
}

void Keyboard::setRightHandLaser(unsigned int rightHandLaser) {
    _handLaserLock.withWriteLock([&] {
        _rightHandLaser = rightHandLaser;
    });
}

void Keyboard::handleTriggerEnd(const QUuid& id, const PointerEvent& event) {
    if (!shouldProcessEntityAndPointerEvent(event)) {
        return;
    }

    auto& keyboardLayer = _keyboardLayers[_layerIndex];
    auto search = keyboardLayer.find(id);

    if (search == keyboardLayer.end()) {
        return;
    }

    Key& key = search.value();

    EntityItemProperties properties;
    properties.setLocalPosition(key.getCurrentLocalPosition());
    DependencyManager::get<EntityScriptingInterface>()->editEntity(id, properties);

    key.setIsPressed(false);
    if (key.timerFinished() && getPreferMalletsOverLasers()) {
        key.startTimer(KEY_PRESS_TIMEOUT_MS);
    }

    auto selection = DependencyManager::get<SelectionScriptingInterface>();
    selection->removeFromSelectedItemsList(KEY_PRESSED_HIGHLIGHT, "entity", id);
}

void Keyboard::handleTriggerContinue(const QUuid& id, const PointerEvent& event) {
    if (!shouldProcessEntityAndPointerEvent(event)) {
        return;
    }

    auto& keyboardLayer = _keyboardLayers[_layerIndex];
    auto search = keyboardLayer.find(id);

    if (search == keyboardLayer.end()) {
        return;
    }

    Key& key = search.value();
    if (!key.isPressed() && getPreferMalletsOverLasers()) {
        unsigned int pointerID = event.getID();
        auto pointerManager = DependencyManager::get<PointerManager>();
        auto pickResult = pointerManager->getPrevPickResult(pointerID);
        auto stylusPickResult = std::dynamic_pointer_cast<StylusPickResult>(pickResult);
        float distance = stylusPickResult->distance;

        static const float PENETRATION_THRESHOLD = 0.025f;
        if (distance < PENETRATION_THRESHOLD) {
            static const float Z_OFFSET = 0.002f;

            auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
            EntityPropertyFlags desiredProperties;
            desiredProperties += PROP_ROTATION;
            glm::quat orientation = entityScriptingInterface->getEntityProperties(id, desiredProperties).getRotation();
            glm::vec3 yAxis = orientation * Z_AXIS;
            glm::vec3 yOffset = yAxis * Z_OFFSET;
            glm::vec3 localPosition = key.getCurrentLocalPosition() - yOffset;

            EntityItemProperties properties;
            properties.setLocalPosition(localPosition);
            entityScriptingInterface->editEntity(id, properties);
            key.setIsPressed(true);
        }
    }
}

void Keyboard::handleHoverBegin(const QUuid& id, const PointerEvent& event) {
    if (!shouldProcessEntityAndPointerEvent(event)) {
        return;
    }

    auto& keyboardLayer = _keyboardLayers[_layerIndex];
    auto search = keyboardLayer.find(id);

    if (search == keyboardLayer.end()) {
        return;
    }

    auto selection = DependencyManager::get<SelectionScriptingInterface>();
    selection->addToSelectedItemsList(KEY_HOVER_HIGHLIGHT, "entity", id);
}

void Keyboard::handleHoverEnd(const QUuid& id, const PointerEvent& event) {
    if (!shouldProcessEntityAndPointerEvent(event)) {
        return;
    }

    auto& keyboardLayer = _keyboardLayers[_layerIndex];
    auto search = keyboardLayer.find(id);

    if (search == keyboardLayer.end()) {
        return;
    }

    auto selection = DependencyManager::get<SelectionScriptingInterface>();
    selection->removeFromSelectedItemsList(KEY_HOVER_HIGHLIGHT, "entity", id);
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
        auto requestData = request->getData();

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

        auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
        {
            glm::vec3 dimensions = vec3FromVariant(anchorObject["dimensions"].toVariant());

            EntityItemProperties properties;
            properties.setType(EntityTypes::Box);
            properties.setName("KeyboardAnchor");
            properties.setVisible(false);
            properties.getGrab().setGrabbable(true);
            properties.setIgnorePickIntersection(false);
            properties.setDimensions(dimensions);
            properties.setPosition(vec3FromVariant(anchorObject["position"].toVariant()));
            properties.setRotation(quatFromVariant(anchorObject["rotation"].toVariant()));

            Anchor anchor;
            anchor.entityID = entityScriptingInterface->addEntityInternal(properties, entity::HostType::LOCAL);
            anchor.originalDimensions = dimensions;
            _anchor = anchor;
        }

        {
            QJsonObject backPlateObject = jsonObject["backPlate"].toObject();
            glm::vec3 position = vec3FromVariant(backPlateObject["position"].toVariant());
            glm::vec3 dimensions = vec3FromVariant(backPlateObject["dimensions"].toVariant());
            glm::quat rotation = quatFromVariant(backPlateObject["rotation"].toVariant());

            EntityItemProperties properties;
            properties.setType(EntityTypes::Box);
            properties.setName("Keyboard-BackPlate");
            properties.setVisible(true);
            properties.getGrab().setGrabbable(false);
            properties.setAlpha(0.0f);
            properties.setIgnorePickIntersection(false);
            properties.setDimensions(dimensions);
            properties.setPosition(position);
            properties.setRotation(rotation);
            properties.setParentID(_anchor.entityID);

            BackPlate backPlate;
            backPlate.entityID = entityScriptingInterface->addEntityInternal(properties, entity::HostType::LOCAL);
            backPlate.dimensions = dimensions;
            glm::quat anchorEntityInverseWorldOrientation = glm::inverse(rotation);
            glm::vec3 anchorEntityLocalTranslation = anchorEntityInverseWorldOrientation * -position;
            backPlate.localPosition = (anchorEntityInverseWorldOrientation * position) + anchorEntityLocalTranslation;
            _backPlate = backPlate;
        }

        const QJsonArray& keyboardLayers = jsonObject["layers"].toArray();
        int keyboardLayerCount = keyboardLayers.size();
        _keyboardLayers.reserve(keyboardLayerCount);
        for (int keyboardLayerIndex = 0; keyboardLayerIndex < keyboardLayerCount; keyboardLayerIndex++) {
            const QJsonValue& keyboardLayer = keyboardLayers[keyboardLayerIndex].toArray();

            QHash<QUuid, Key> keyboardLayerKeys;
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

                EntityItemProperties properties;
                properties.setType(EntityTypes::Model);
                properties.setDimensions(vec3FromVariant(keyboardKeyValue["dimensions"].toVariant()));
                properties.setPosition(vec3FromVariant(keyboardKeyValue["position"].toVariant()));
                properties.setVisible(false);
                properties.setEmissive(true);
                properties.setParentID(_anchor.entityID);
                properties.setModelURL(url);
                properties.setTextures(QString(QJsonDocument::fromVariant(textureMap).toJson()));
                properties.getGrab().setGrabbable(false);
                properties.setLocalRotation(quatFromVariant(keyboardKeyValue["localOrientation"].toVariant()));
                QUuid id = entityScriptingInterface->addEntityInternal(properties, entity::HostType::LOCAL);

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
                key.setID(id);
                key.setKeyString(keyString);
                key.saveDimensionsAndLocalPosition();

                _itemsToIgnore.insert(key.getID());
                keyboardLayerKeys.insert(id, key);
            }

            _keyboardLayers.push_back(keyboardLayerKeys);

        }

        {
            QJsonObject displayTextObject = jsonObject["textDisplay"].toObject();
            glm::vec3 dimensions = vec3FromVariant(displayTextObject["dimensions"].toVariant());
            glm::vec3 localPosition = vec3FromVariant(displayTextObject["localPosition"].toVariant());
            float lineHeight = (float)displayTextObject["lineHeight"].toDouble();

            EntityItemProperties properties;
            properties.setType(EntityTypes::Text);
            properties.setDimensions(dimensions);
            properties.setLocalPosition(localPosition);
            properties.setLocalRotation(quatFromVariant(displayTextObject["localOrientation"].toVariant()));
            properties.setLeftMargin((float)displayTextObject["leftMargin"].toDouble());
            properties.setRightMargin((float)displayTextObject["rightMargin"].toDouble());
            properties.setTopMargin((float)displayTextObject["topMargin"].toDouble());
            properties.setBottomMargin((float)displayTextObject["bottomMargin"].toDouble());
            properties.setLineHeight((float)displayTextObject["lineHeight"].toDouble());
            properties.setVisible(false);
            properties.setEmissive(true);
            properties.getGrab().setGrabbable(false);
            properties.setText("");
            properties.setTextAlpha(1.0f);
            properties.setBackgroundAlpha(0.7f);
            properties.setParentID(_anchor.entityID);

            TextDisplay textDisplay;
            textDisplay.entityID = entityScriptingInterface->addEntityInternal(properties, entity::HostType::LOCAL);
            textDisplay.localPosition = localPosition;
            textDisplay.dimensions = dimensions;
            textDisplay.lineHeight = lineHeight;
            _textDisplay = textDisplay;
        }

        _ignoreItemsLock.withWriteLock([&] {
            _itemsToIgnore.insert(_textDisplay.entityID);
            _itemsToIgnore.insert(_anchor.entityID);
        });
        _layerIndex = 0;
        addIncludeItemsToMallets();
    });

    request->send();
}


QUuid Keyboard::getAnchorID() {
    return _ignoreItemsLock.resultWithReadLock<QUuid>([&] {
        return _anchor.entityID;
    });
}

bool Keyboard::shouldProcessEntity() const {
    return (!_keyboardLayers.empty() && isLayerSwitchTimerFinished());
}

QSet<QUuid> Keyboard::getKeyIDs() {
    return _ignoreItemsLock.resultWithReadLock<QSet<QUuid>>([&] {
        return _itemsToIgnore;
    });
}

void Keyboard::clearKeyboardKeys() {
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    for (const auto& keyboardLayer: _keyboardLayers) {
        for (auto iter = keyboardLayer.begin(); iter != keyboardLayer.end(); iter++) {
            entityScriptingInterface->deleteEntity(iter.key());
        }
    }

    entityScriptingInterface->deleteEntity(_anchor.entityID);
    entityScriptingInterface->deleteEntity(_textDisplay.entityID);
    entityScriptingInterface->deleteEntity(_backPlate.entityID);

    _keyboardLayers.clear();

    _ignoreItemsLock.withWriteLock([&] {
        _itemsToIgnore.clear();
    });
}

void Keyboard::enableStylus() {
    if (getPreferMalletsOverLasers()) {
        enableRightMallet();
        enableLeftMallet();
    }
}

void Keyboard::enableRightMallet() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    pointerManager->setRenderState(_rightHandStylus, "events on");
    pointerManager->enablePointer(_rightHandStylus);
}

void Keyboard::enableLeftMallet() {
     auto pointerManager = DependencyManager::get<PointerManager>();
     pointerManager->setRenderState(_leftHandStylus, "events on");
     pointerManager->enablePointer(_leftHandStylus);
}

void Keyboard::disableLeftMallet() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    pointerManager->setRenderState(_leftHandStylus, "events off");
    pointerManager->disablePointer(_leftHandStylus);
}

void Keyboard::disableRightMallet() {
    auto pointerManager = DependencyManager::get<PointerManager>();
    pointerManager->setRenderState(_rightHandStylus, "events off");
    pointerManager->disablePointer(_rightHandStylus);
}

bool Keyboard::containsID(const QUuid& id) const {
    return resultWithReadLock<bool>([&] {
        return _itemsToIgnore.contains(id) || _backPlate.entityID == id;
    });
}
