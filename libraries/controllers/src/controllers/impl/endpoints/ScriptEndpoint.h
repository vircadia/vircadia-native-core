//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_ScriptEndpoint_h
#define hifi_Controllers_ScriptEndpoint_h

#include <QtScript/QScriptValue>

#include "../Endpoint.h"

namespace controller {

class ScriptEndpoint : public Endpoint {
    Q_OBJECT;
public:
    ScriptEndpoint(const QScriptValue& callable)
        : Endpoint(Input::INVALID_INPUT), _callable(callable) {
    }

    virtual float value();
    virtual void apply(float newValue, const Pointer& source);

protected:
    Q_INVOKABLE void updateValue();
    Q_INVOKABLE virtual void internalApply(float newValue, int sourceID);
private:
    QScriptValue _callable;
    float _lastValueRead { 0.0f };
    float _lastValueWritten { 0.0f };
};

}

#endif
