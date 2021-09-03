//
//  ScriptValueIteratorQtWrapper.h
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 8/29/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ScriptValueIteratorQtWrapper_h
#define hifi_ScriptValueIteratorQtWrapper_h

#include <QtCore/QPointer>
#include <QtScript/QScriptValueIterator>

#include "../ScriptValueIterator.h"
#include "ScriptEngineQtScript.h"
#include "ScriptValueQtWrapper.h"

/// [QtScript] Implements ScriptValueIterator for QtScript and translates calls for QScriptValueIterator
class ScriptValueIteratorQtWrapper : public ScriptValueIterator {
public: // construction
    inline ScriptValueIteratorQtWrapper(ScriptEngineQtScript* engine, const ScriptValue& object) :
        _engine(engine), _value(ScriptValueQtWrapper::fullUnwrap(engine, object)) {}
    inline ScriptValueIteratorQtWrapper(ScriptEngineQtScript* engine, const QScriptValue& object) :
        _engine(engine), _value(object) {}

public:  // ScriptValueIterator implementation
    virtual ScriptValue::PropertyFlags flags() const override;
    virtual bool hasNext() const override;
    virtual QString name() const override;
    virtual void next() override;
    virtual ScriptValue value() const override;

private: // storage
    QPointer<ScriptEngineQtScript> _engine;
    QScriptValueIterator _value;
};

#endif  // hifi_ScriptValueIteratorQtWrapper_h

/// @}
