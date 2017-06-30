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

/// handles scripting of input controller commands from JS
class ControllerScriptingInterface : public controller::ScriptingInterface {
    Q_OBJECT


public:    
    virtual ~ControllerScriptingInterface() {}

    void emitKeyPressEvent(QKeyEvent* event);
    void emitKeyReleaseEvent(QKeyEvent* event);
    
    void handleMetaEvent(HFMetaEvent* event);

    void emitMouseMoveEvent(QMouseEvent* event);
    void emitMousePressEvent(QMouseEvent* event); 
    void emitMouseDoublePressEvent(QMouseEvent* event);
    void emitMouseReleaseEvent(QMouseEvent* event);

    void emitTouchBeginEvent(const TouchEvent& event);
    void emitTouchEndEvent(const TouchEvent& event); 
    void emitTouchUpdateEvent(const TouchEvent& event);
    
    void emitWheelEvent(QWheelEvent* event);

    bool isKeyCaptured(QKeyEvent* event) const;
    bool isKeyCaptured(const KeyEvent& event) const;
    bool isJoystickCaptured(int joystickIndex) const;
    bool areEntityClicksCaptured() const;

public slots:

    virtual void captureKeyEvents(const KeyEvent& event);
    virtual void releaseKeyEvents(const KeyEvent& event);

    virtual void captureJoystick(int joystickIndex);
    virtual void releaseJoystick(int joystickIndex);

    virtual void captureEntityClickEvents();
    virtual void releaseEntityClickEvents();

    virtual glm::vec2 getViewportDimensions() const;
    virtual QVariant getRecommendedOverlayRect() const;

signals:
    void keyPressEvent(const KeyEvent& event);
    void keyReleaseEvent(const KeyEvent& event);

    void actionStartEvent(const HFActionEvent& event);
    void actionEndEvent(const HFActionEvent& event);

    void backStartEvent();
    void backEndEvent();

    void mouseMoveEvent(const MouseEvent& event);
    void mousePressEvent(const MouseEvent& event);
    void mouseDoublePressEvent(const MouseEvent& event);
    void mouseReleaseEvent(const MouseEvent& event);

    void touchBeginEvent(const TouchEvent& event);
    void touchEndEvent(const TouchEvent& event);
    void touchUpdateEvent(const TouchEvent& event);

    void wheelEvent(const WheelEvent& event);

private:
    QString sanatizeName(const QString& name); /// makes a name clean for inclusing in JavaScript

    QMultiMap<int,KeyEvent> _capturedKeys;
    QSet<int> _capturedJoysticks;
    bool _captureEntityClicks;

    using InputKey = controller::InputController::Key;
};

const int NUMBER_OF_SPATIALCONTROLS_PER_PALM = 2; // the hand and the tip
const int NUMBER_OF_JOYSTICKS_PER_PALM = 1;
const int NUMBER_OF_TRIGGERS_PER_PALM = 1;
const int NUMBER_OF_BUTTONS_PER_PALM = 6;
const int PALM_SPATIALCONTROL = 0;
const int TIP_SPATIALCONTROL = 1;


#endif // hifi_ControllerScriptingInterface_h
