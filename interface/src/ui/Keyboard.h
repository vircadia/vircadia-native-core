//
//  Keyboard.h
//  interface/src/scripting
//
//  Created by Dante Ruiz on 2018-08-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Keyboard_h
#define hifi_Keyboard_h

#include <map>
#include <memory>
#include <vector>

#include <QtCore/QObject>
#include <QTimer>
#include <QHash>
#include <QUuid>
#include <DependencyManager.h>
#include <Sound.h>
#include <shared/ReadWriteLockable.h>
#include <SettingHandle.h>

class PointerEvent;


class Key {
public:
    Key() = default;
    ~Key() = default;

    enum Type {
        CHARACTER,
        CAPS,
        CLOSE,
        LAYER,
        BACKSPACE,
        SPACE,
        ENTER
    };

    static Key::Type getKeyTypeFromString(const QString& keyTypeString);

    QUuid getID() const { return _keyID; }
    void setID(const QUuid& id) { _keyID = id; }

    void startTimer(int time);
    bool timerFinished();

    void setKeyString(QString keyString) { _keyString = keyString; }
    QString getKeyString(bool toUpper) const;
    int getScanCode(bool toUpper) const;

    bool isPressed() const { return _pressed; }
    void setIsPressed(bool pressed) { _pressed = pressed; }

    void setSwitchToLayerIndex(int layerIndex) { _switchToLayer = layerIndex; }
    int getSwitchToLayerIndex() const { return _switchToLayer; }

    Type getKeyType() const { return _type; }
    void setKeyType(Type type) { _type = type; }

    glm::vec3 getCurrentLocalPosition() const { return _currentLocalPosition; }

    void saveDimensionsAndLocalPosition();

    void scaleKey(float sensorToWorldScale);
private:
    Type _type { Type::CHARACTER };

    int _switchToLayer { 0 };
    bool _pressed { false };

    QUuid _keyID;
    QString _keyString;

    glm::vec3 _originalLocalPosition;
    glm::vec3 _originalDimensions;
    glm::vec3 _currentLocalPosition;
    bool _originalDimensionsAndLocalPositionSaved { false };

    std::shared_ptr<QTimer> _timer { std::make_shared<QTimer>() };
};

class Keyboard : public QObject, public Dependency, public ReadWriteLockable {
    Q_OBJECT
public:
    Keyboard();
    void createKeyboard();
    void registerKeyboardHighlighting();
    bool isRaised() const;
    void setRaised(bool raised);
    void setResetKeyboardPositionOnRaise(bool reset);
    bool isPassword() const;
    void setPassword(bool password);
    void enableRightMallet();
    void scaleKeyboard(float sensorToWorldScale);
    void enableLeftMallet();
    void disableRightMallet();
    void disableLeftMallet();

    void setLeftHandLaser(unsigned int leftHandLaser);
    void setRightHandLaser(unsigned int rightHandLaser);

    void setPreferMalletsOverLasers(bool preferMalletsOverLasers);
    bool getPreferMalletsOverLasers() const;

    bool getUse3DKeyboard() const;
    void setUse3DKeyboard(bool use);
    bool containsID(const QUuid& id) const;

    void loadKeyboardFile(const QString& keyboardFile);
    QSet<QUuid> getKeyIDs();
    QUuid getAnchorID();

public slots:
    void handleTriggerBegin(const QUuid& id, const PointerEvent& event);
    void handleTriggerEnd(const QUuid& id, const PointerEvent& event);
    void handleTriggerContinue(const QUuid& id, const PointerEvent& event);
    void handleHoverBegin(const QUuid& id, const PointerEvent& event);
    void handleHoverEnd(const QUuid& id, const PointerEvent& event);

private:
    struct Anchor {
        QUuid entityID;
        glm::vec3 originalDimensions;
    };

    struct BackPlate {
        QUuid entityID;
        glm::vec3 dimensions;
        glm::vec3 localPosition;
    };

    struct TextDisplay {
        float lineHeight;
        QUuid entityID;
        glm::vec3 localPosition;
        glm::vec3 dimensions;
    };

    void raiseKeyboard(bool raise) const;
    void raiseKeyboardAnchor(bool raise) const;
    void enableStylus();
    void disableStylus();
    void enableSelectionLists();
    void disableSelectionLists();

    void setLayerIndex(int layerIndex);
    void clearKeyboardKeys();
    void switchToLayer(int layerIndex);
    void updateTextDisplay();
    bool shouldProcessEntityAndPointerEvent(const PointerEvent& event) const;
    bool shouldProcessPointerEvent(const PointerEvent& event) const;
    bool shouldProcessEntity() const;
    void addIncludeItemsToMallets();

    void startLayerSwitchTimer();
    bool isLayerSwitchTimerFinished() const;

    bool _raised { false };
    bool _resetKeyboardPositionOnRaise { true };
    bool _password { false };
    bool _capsEnabled { false };
    int _layerIndex { 0 };
    Setting::Handle<bool> _preferMalletsOverLasers { "preferMalletsOverLaser", true };
    unsigned int _leftHandStylus { 0 };
    unsigned int _rightHandStylus { 0 };
    unsigned int _leftHandLaser { 0 };
    unsigned int _rightHandLaser { 0 };
    SharedSoundPointer _keySound { nullptr };
    std::shared_ptr<QTimer> _layerSwitchTimer { std::make_shared<QTimer>() };

    mutable ReadWriteLockable _use3DKeyboardLock;
    mutable ReadWriteLockable _handLaserLock;
    mutable ReadWriteLockable _preferMalletsOverLasersSettingLock;
    mutable ReadWriteLockable _ignoreItemsLock;

    Setting::Handle<bool> _use3DKeyboard { "use3DKeyboard", true };

    QString _typedCharacters;
    TextDisplay _textDisplay;
    Anchor _anchor;
    BackPlate _backPlate;

    QSet<QUuid> _itemsToIgnore;
    std::vector<QHash<QUuid, Key>> _keyboardLayers;

    bool _created { false };
};

#endif
