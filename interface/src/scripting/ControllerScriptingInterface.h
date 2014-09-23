//
//  ControllerScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 12/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ControllerScriptingInterface_h
#define hifi_ControllerScriptingInterface_h

#include <QtCore/QObject>

#include <AbstractControllerScriptingInterface.h>
class PalmData;


class InputController : public  AbstractInputController {
    Q_OBJECT

public:
    InputController(int deviceTrackerId, int subTrackerId, QObject* parent = NULL);

    virtual void update();
    virtual Key getKey() const;

public slots:

    virtual bool isActive() const { return _isActive; }
    virtual glm::vec3 getAbsTranslation() const { return _eventCache.absTranslation; }
    virtual glm::quat getAbsRotation() const    { return _eventCache.absRotation; }
    virtual glm::vec3 getLocTranslation() const { return _eventCache.locTranslation; }
    virtual glm::quat getLocRotation() const    { return _eventCache.locRotation; }

private:

    int  _deviceTrackerId;
    int  _subTrackerId;

    // cache for the spatial
    SpatialEvent    _eventCache;
    bool            _isActive;

signals:
};


/// handles scripting of input controller commands from JS
class ControllerScriptingInterface : public AbstractControllerScriptingInterface {
    Q_OBJECT

public:    
    ControllerScriptingInterface();
    void emitKeyPressEvent(QKeyEvent* event) { emit keyPressEvent(KeyEvent(*event)); }
    void emitKeyReleaseEvent(QKeyEvent* event) { emit keyReleaseEvent(KeyEvent(*event)); }

    void emitMouseMoveEvent(QMouseEvent* event, unsigned int deviceID = 0) { emit mouseMoveEvent(MouseEvent(*event, deviceID)); }
    void emitMousePressEvent(QMouseEvent* event, unsigned int deviceID = 0) { emit mousePressEvent(MouseEvent(*event, deviceID)); }
    void emitMouseReleaseEvent(QMouseEvent* event, unsigned int deviceID = 0) { emit mouseReleaseEvent(MouseEvent(*event, deviceID)); }

    void emitTouchBeginEvent(const TouchEvent& event) { emit touchBeginEvent(event); }
    void emitTouchEndEvent(const TouchEvent& event) { emit touchEndEvent(event); }
    void emitTouchUpdateEvent(const TouchEvent& event) { emit touchUpdateEvent(event); }
    
    void emitWheelEvent(QWheelEvent* event) { emit wheelEvent(*event); }

    bool isKeyCaptured(QKeyEvent* event) const;
    bool isKeyCaptured(const KeyEvent& event) const;
    bool isMouseCaptured() const { return _mouseCaptured; }
    bool isTouchCaptured() const { return _touchCaptured; }
    bool isWheelCaptured() const { return _wheelCaptured; }
    bool isJoystickCaptured(int joystickIndex) const;

    void updateInputControllers();

public slots:
    virtual bool isPrimaryButtonPressed() const;
    virtual glm::vec2 getPrimaryJoystickPosition() const;

    virtual int getNumberOfButtons() const;
    virtual bool isButtonPressed(int buttonIndex) const;

    virtual int getNumberOfTriggers() const;
    virtual float getTriggerValue(int triggerIndex) const;

    virtual int getNumberOfJoysticks() const;
    virtual glm::vec2 getJoystickPosition(int joystickIndex) const;

    virtual int getNumberOfSpatialControls() const;
    virtual glm::vec3 getSpatialControlPosition(int controlIndex) const;
    virtual glm::vec3 getSpatialControlVelocity(int controlIndex) const;
    virtual glm::vec3 getSpatialControlNormal(int controlIndex) const;
    virtual glm::quat getSpatialControlRawRotation(int controlIndex) const;
    virtual void captureKeyEvents(const KeyEvent& event);
    virtual void releaseKeyEvents(const KeyEvent& event);

    virtual void captureMouseEvents() { _mouseCaptured = true; }
    virtual void releaseMouseEvents() { _mouseCaptured = false; }

    virtual void captureTouchEvents() { _touchCaptured = true; }
    virtual void releaseTouchEvents() { _touchCaptured = false; }

    virtual void captureWheelEvents() { _wheelCaptured = true; }
    virtual void releaseWheelEvents() { _wheelCaptured = false; }

    virtual void captureJoystick(int joystickIndex);
    virtual void releaseJoystick(int joystickIndex);

    virtual glm::vec2 getViewportDimensions() const;

    /// Factory to create an InputController
    virtual AbstractInputController* createInputController(const QString& deviceName, const QString& tracker);
    JoystickInputController* joystick() const;

    virtual void releaseInputController(AbstractInputController* input);

private:
    const PalmData* getPrimaryPalm() const;
    const PalmData* getPalm(int palmIndex) const;
    int getNumberOfActivePalms() const;
    const PalmData* getActivePalm(int palmIndex) const;
    
    bool _mouseCaptured;
    bool _touchCaptured;
    bool _wheelCaptured;
    QMultiMap<int,KeyEvent> _capturedKeys;
    QSet<int> _capturedJoysticks;

    typedef std::map< AbstractInputController::Key, AbstractInputController* > InputControllerMap;
    InputControllerMap _inputControllers;
};

const int NUMBER_OF_SPATIALCONTROLS_PER_PALM = 2; // the hand and the tip
const int NUMBER_OF_JOYSTICKS_PER_PALM = 1;
const int NUMBER_OF_TRIGGERS_PER_PALM = 1;
const int NUMBER_OF_BUTTONS_PER_PALM = 6;
const int PALM_SPATIALCONTROL = 0;
const int TIP_SPATIALCONTROL = 1;


#endif // hifi_ControllerScriptingInterface_h
