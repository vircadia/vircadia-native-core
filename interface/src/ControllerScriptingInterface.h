
//
//  ControllerScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/17/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ControllerScriptingInterface__
#define __hifi__ControllerScriptingInterface__

#include <QtCore/QObject>

#include <AbstractControllerScriptingInterface.h>
class PalmData;

/// handles scripting of input controller commands from JS
class ControllerScriptingInterface : public AbstractControllerScriptingInterface {
    Q_OBJECT

public:    
    void emitKeyPressEvent(QKeyEvent* event) { emit keyPressEvent(KeyEvent(*event)); }
    void emitKeyReleaseEvent(QKeyEvent* event) { emit keyReleaseEvent(KeyEvent(*event)); }

    void emitMouseMoveEvent(QMouseEvent* event) { emit mouseMoveEvent(MouseEvent(*event)); }
    void emitMousePressEvent(QMouseEvent* event) { emit mousePressEvent(MouseEvent(*event)); }
    void emitMouseReleaseEvent(QMouseEvent* event) { emit mouseReleaseEvent(MouseEvent(*event)); }

    void emitTouchBeginEvent(QTouchEvent* event) { emit touchBeginEvent(*event); }
    void emitTouchEndEvent(QTouchEvent* event) { emit touchEndEvent(*event); }
    void emitTouchUpdateEvent(QTouchEvent* event) { emit touchUpdateEvent(*event); }
    void emitWheelEvent(QWheelEvent* event) { emit wheelEvent(*event); }

    bool isKeyCaptured(QKeyEvent* event) const;
    bool isKeyCaptured(const KeyEvent& event) const;
    bool isMouseCaptured() const { return _mouseCaptured; }
    bool isTouchCaptured() const { return _touchCaptured; }
    bool isWheelCaptured() const { return _wheelCaptured; }


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

    virtual void captureKeyEvents(const KeyEvent& event);
    virtual void releaseKeyEvents(const KeyEvent& event);

    virtual void captureMouseEvents() { _mouseCaptured = true; }
    virtual void releaseMouseEvents() { _mouseCaptured = false; }

    virtual void captureTouchEvents() { _touchCaptured = true; }
    virtual void releaseTouchEvents() { _touchCaptured = false; }

    virtual void captureWheelEvents() { _wheelCaptured = true; }
    virtual void releaseWheelEvents() { _wheelCaptured = false; }

// The following signals are defined by AbstractControllerScriptingInterface
//
// signals:
//      void keyPressEvent();
//      void keyPressEvent();

private:
    const PalmData* getPrimaryPalm() const;
    const PalmData* getPalm(int palmIndex) const;
    int getNumberOfActivePalms() const;
    const PalmData* getActivePalm(int palmIndex) const;
    
    bool _mouseCaptured;
    bool _touchCaptured;
    bool _wheelCaptured;
    QMultiMap<int,KeyEvent> _capturedKeys;
};

const int NUMBER_OF_SPATIALCONTROLS_PER_PALM = 2; // the hand and the tip
const int NUMBER_OF_JOYSTICKS_PER_PALM = 1;
const int NUMBER_OF_TRIGGERS_PER_PALM = 1;
const int NUMBER_OF_BUTTONS_PER_PALM = 6;
const int PALM_SPATIALCONTROL = 0;
const int TIP_SPATIALCONTROL = 1;

#endif /* defined(__hifi__ControllerScriptingInterface__) */
