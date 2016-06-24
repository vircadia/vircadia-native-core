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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "InputDevice.h"
#include "StandardControls.h"

namespace controller {

class StandardController : public QObject, public InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)

public:
    virtual EndpointPointer createEndpoint(const Input& input) const override;
    virtual Input::NamedVector getAvailableInputs() const override;
    virtual QStringList getDefaultMappingConfigs() const override;
    virtual void focusOutEvent() override;

    StandardController(); 
};

}

#endif // hifi_StandardController_h
