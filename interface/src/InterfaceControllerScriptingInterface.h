//
//  InterfaceControllerScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/17/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__InterfaceControllerScriptingInterface__
#define __hifi__InterfaceControllerScriptingInterface__

#include <QtCore/QObject>

#include <ControllerScriptingInterface.h>

/// handles scripting of input controller commands from JS
class InterfaceControllerScriptingInterface : public ControllerScriptingInterface {
    Q_OBJECT

public slots:
    virtual bool isPrimaryButtonPressed() const;
    virtual glm::vec2 getPrimaryJoystickPosition() const;

private:
    const PalmData* getPrimaryPalm() const;
};

#endif /* defined(__hifi__InterfaceControllerScriptingInterface__) */
