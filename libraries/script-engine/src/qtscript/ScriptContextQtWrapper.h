//
//  ScriptContextQtWrapper.h
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 5/22/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ScriptContextQtWrapper_h
#define hifi_ScriptContextQtWrapper_h

#include <QtCore/QString>
#include <QtScript/QScriptContextInfo>

#include "../ScriptContext.h"
#include "../ScriptValue.h"

class QScriptContext;
class ScriptEngineQtScript;

/// [QtScript] Implements ScriptContext for QtScript and translates calls for QScriptContextInfo
class ScriptContextQtWrapper : public ScriptContext {
public: // construction
    inline ScriptContextQtWrapper(ScriptEngineQtScript* engine, QScriptContext* context) : _engine(engine), _context(context) {}
    static ScriptContextQtWrapper* unwrap(ScriptContext* val);
    inline QScriptContext* toQtValue() const { return _context; }

public: // ScriptContext implementation
    virtual int argumentCount() const;
    virtual ScriptValue argument(int index) const;
    virtual QStringList backtrace() const;
    virtual ScriptValue callee() const;
    virtual ScriptEnginePointer engine() const;
    virtual ScriptFunctionContextPointer functionContext() const;
    virtual ScriptContextPointer parentContext() const;
    virtual ScriptValue thisObject() const;
    virtual ScriptValue throwError(const QString& text);
    virtual ScriptValue throwValue(const ScriptValue& value);

private: // storage
    QScriptContext* _context;
    ScriptEngineQtScript* _engine;
};

class ScriptFunctionContextQtWrapper : public ScriptFunctionContext {
public:  // construction
    inline ScriptFunctionContextQtWrapper(QScriptContext* context) : _value(context) {}

public:  // ScriptFunctionContext implementation
    virtual QString fileName() const;
    virtual QString functionName() const;
    virtual FunctionType functionType() const;
    virtual int lineNumber() const;

private: // storage
    QScriptContextInfo _value;
};

#endif  // hifi_ScriptContextQtWrapper_h

/// @}
