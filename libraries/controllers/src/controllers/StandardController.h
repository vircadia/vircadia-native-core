//
//  StandardController.h
//  input-plugins/src/input-plugins
//
//  Created by Brad Hefta-Gaub on 2015-10-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StandardController_h
#define hifi_StandardController_h

#include <qobject.h>
#include <qvector.h>

#include "InputDevice.h"

#include "StandardControls.h"

typedef std::shared_ptr<StandardController> StandardControllerPointer;

class StandardController : public QObject, public InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)

public:

    const QString& getName() const { return _name; }

    // Device functions
    virtual void registerToUserInputMapper(UserInputMapper& mapper) override;
    virtual void assignDefaultInputMapping(UserInputMapper& mapper) override;
    virtual void update(float deltaTime, bool jointsCaptured) override;
    virtual void focusOutEvent() override;
    
    StandardController() : InputDevice("Standard") {}
    ~StandardController();
    
    UserInputMapper::Input makeInput(controller::StandardButtonChannel button);
    UserInputMapper::Input makeInput(controller::StandardAxisChannel axis);
    UserInputMapper::Input makeInput(controller::StandardPoseChannel pose);

private:
};

#endif // hifi_StandardController_h
