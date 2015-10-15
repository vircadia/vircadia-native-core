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

#include <controllers/UserInputMapper.h>
#include <controllers/ScriptingInterface.h>

#include <HFActionEvent.h>
#include <KeyEvent.h>
#include <MouseEvent.h>
#include <SpatialEvent.h>
#include <TouchEvent.h>
#include <WheelEvent.h>
class ScriptEngine;

class PalmData;

class InputController : public  controller::InputController {
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
    uint  _subTrackerId;

    // cache for the spatial
    SpatialEvent    _eventCache;
    bool            _isActive;

signals:
};
 

/// handles scripting of input controller commands from JS
class ControllerScriptingInterface : public controller::ScriptingInterface {
    Q_OBJECT


public:    
    ControllerScriptingInterface();
    ~ControllerScriptingInterface();

    Q_INVOKABLE QVector<UserInputMapper::Action> getAllActions();

    Q_INVOKABLE bool addInputChannel(UserInputMapper::InputChannel inputChannel);
    Q_INVOKABLE bool removeInputChannel(UserInputMapper::InputChannel inputChannel);
    Q_INVOKABLE QVector<UserInputMapper::InputChannel> getInputChannelsForAction(UserInputMapper::Action action);

    Q_INVOKABLE QVector<UserInputMapper::InputPair> getAvailableInputs(unsigned int device);
    Q_INVOKABLE QVector<UserInputMapper::InputChannel> getAllInputsForDevice(unsigned int device);

    Q_INVOKABLE QString getDeviceName(unsigned int device);

    Q_INVOKABLE float getActionValue(int action);

    Q_INVOKABLE void resetDevice(unsigned int device);
    Q_INVOKABLE void resetAllDeviceBindings();
    Q_INVOKABLE int findDevice(QString name);
    Q_INVOKABLE QVector<QString> getDeviceNames();

    Q_INVOKABLE int findAction(QString actionName);
    Q_INVOKABLE QVector<QString> getActionNames() const;


    virtual void registerControllerTypes(QScriptEngine* engine);
    
    void emitKeyPressEvent(QKeyEvent* event);
    void emitKeyReleaseEvent(QKeyEvent* event);
    
    void handleMetaEvent(HFMetaEvent* event);

    void emitMouseMoveEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void emitMousePressEvent(QMouseEvent* event, unsigned int deviceID = 0); 
    void emitMouseDoublePressEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void emitMouseReleaseEvent(QMouseEvent* event, unsigned int deviceID = 0);

    void emitTouchBeginEvent(const TouchEvent& event);
    void emitTouchEndEvent(const TouchEvent& event); 
    void emitTouchUpdateEvent(const TouchEvent& event);
    
    void emitWheelEvent(QWheelEvent* event);

    bool isKeyCaptured(QKeyEvent* event) const;
    bool isKeyCaptured(const KeyEvent& event) const;
    bool isMouseCaptured() const { return _mouseCaptured; }
    bool isTouchCaptured() const { return _touchCaptured; }
    bool isWheelCaptured() const { return _wheelCaptured; }
    bool areActionsCaptured() const { return _actionsCaptured; }
    bool isJoystickCaptured(int joystickIndex) const;

    virtual void update() override;

public slots:

    virtual void captureKeyEvents(const KeyEvent& event);
    virtual void releaseKeyEvents(const KeyEvent& event);

    virtual void captureMouseEvents() { _mouseCaptured = true; }
    virtual void releaseMouseEvents() { _mouseCaptured = false; }

    virtual void captureTouchEvents() { _touchCaptured = true; }
    virtual void releaseTouchEvents() { _touchCaptured = false; }

    virtual void captureWheelEvents() { _wheelCaptured = true; }
    virtual void releaseWheelEvents() { _wheelCaptured = false; }
    
    virtual void captureActionEvents() { _actionsCaptured = true; }
    virtual void releaseActionEvents() { _actionsCaptured = false; }

    virtual void captureJoystick(int joystickIndex);
    virtual void releaseJoystick(int joystickIndex);

    virtual glm::vec2 getViewportDimensions() const;

    /// Factory to create an InputController
    virtual controller::InputController::Pointer createInputController(const QString& deviceName, const QString& tracker);
    virtual void releaseInputController(controller::InputController::Pointer input);

signals:
    void keyPressEvent(const KeyEvent& event);
    void keyReleaseEvent(const KeyEvent& event);

    void actionStartEvent(const HFActionEvent& event);
    void actionEndEvent(const HFActionEvent& event);

    void backStartEvent();
    void backEndEvent();

    void mouseMoveEvent(const MouseEvent& event, unsigned int deviceID = 0);
    void mousePressEvent(const MouseEvent& event, unsigned int deviceID = 0);
    void mouseDoublePressEvent(const MouseEvent& event, unsigned int deviceID = 0);
    void mouseReleaseEvent(const MouseEvent& event, unsigned int deviceID = 0);

    void touchBeginEvent(const TouchEvent& event);
    void touchEndEvent(const TouchEvent& event);
    void touchUpdateEvent(const TouchEvent& event);

    void wheelEvent(const WheelEvent& event);

    void actionEvent(int action, float state);

private:
    QString sanatizeName(const QString& name); /// makes a name clean for inclusing in JavaScript

    const PalmData* getPrimaryPalm() const;
    const PalmData* getPalm(int palmIndex) const;
    int getNumberOfActivePalms() const;
    const PalmData* getActivePalm(int palmIndex) const;
    
    bool _mouseCaptured;
    bool _touchCaptured;
    bool _wheelCaptured;
    bool _actionsCaptured;
    QMultiMap<int,KeyEvent> _capturedKeys;
    QSet<int> _capturedJoysticks;

    using InputKey = controller::InputController::Key;
    using InputControllerMap = std::map<InputKey, controller::InputController::Pointer>;
    InputControllerMap _inputControllers;
};

const int NUMBER_OF_SPATIALCONTROLS_PER_PALM = 2; // the hand and the tip
const int NUMBER_OF_JOYSTICKS_PER_PALM = 1;
const int NUMBER_OF_TRIGGERS_PER_PALM = 1;
const int NUMBER_OF_BUTTONS_PER_PALM = 6;
const int PALM_SPATIALCONTROL = 0;
const int TIP_SPATIALCONTROL = 1;


#endif // hifi_ControllerScriptingInterface_h
