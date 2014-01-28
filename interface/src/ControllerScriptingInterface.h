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
    void emitKeyPressEvent(QKeyEvent* x) {
            KeyEvent event(x);
            emit keyPressEvent(event);
    }
    
    /**
    void emitKeyReleaseEvent(QKeyEvent* event) { emit keyReleaseEvent(*event); }

    void emitMouseMoveEvent(QMouseEvent* event) { emit mouseMoveEvent(*event); }
    void emitMousePressEvent(QMouseEvent* event) { emit mousePressEvent(*event); }
    void emitMouseReleaseEvent(QMouseEvent* event) { emit mouseReleaseEvent(*event); }

    void emitTouchBeginEvent(QTouchEvent* event) { emit touchBeginEvent(event); }
    void emitTouchEndEvent(QTouchEvent* event) { emit touchEndEvent(event); }
    void emitTouchUpdateEvent(QTouchEvent* event) { emit touchUpdateEvent(event); }
    void emitWheelEvent(QWheelEvent* event) { emit wheelEvent(event); }
     **/

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
};

const int NUMBER_OF_SPATIALCONTROLS_PER_PALM = 2; // the hand and the tip
const int NUMBER_OF_JOYSTICKS_PER_PALM = 1;
const int NUMBER_OF_TRIGGERS_PER_PALM = 1;
const int NUMBER_OF_BUTTONS_PER_PALM = 6;
const int PALM_SPATIALCONTROL = 0;
const int TIP_SPATIALCONTROL = 1;

#endif /* defined(__hifi__ControllerScriptingInterface__) */
