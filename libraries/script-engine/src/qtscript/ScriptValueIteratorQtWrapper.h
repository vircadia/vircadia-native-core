//
//  ScriptValueIteratorQtWrapper.h
//  libraries/script-engine/src
//
//  Created by Heather Anderson on 8/29/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptValueIteratorQtWrapper_h
#define hifi_ScriptValueIteratorQtWrapper_h

//#include <QtCore/QPointer>
#include <QtScript/QScriptValueIterator>

#include "../ScriptValueIterator.h"
#include "ScriptEngineQtScript.h"
#include "ScriptValueQtWrapper.h"

class ScriptValueIteratorQtWrapper : public ScriptValueIterator {
public: // construction
    inline ScriptValueIteratorQtWrapper(ScriptEngineQtScript* engine, const ScriptValuePointer& object) :
        _engine(engine), _value(ScriptValueQtWrapper::fullUnwrap(engine, object)) {}
    inline ScriptValueIteratorQtWrapper(ScriptEngineQtScript* engine, const QScriptValue& object) :
        _engine(engine), _value(object) {}

public:  // ScriptValueIterator implementation
    virtual ScriptValue::PropertyFlags flags() const;
    virtual bool hasNext() const;
    virtual QString name() const;
    virtual void next();
    virtual ScriptValuePointer value() const;

private: // storage
    QPointer<ScriptEngineQtScript> _engine;
    QScriptValueIterator _value;
};

#endif  // hifi_ScriptValueIteratorQtWrapper_h