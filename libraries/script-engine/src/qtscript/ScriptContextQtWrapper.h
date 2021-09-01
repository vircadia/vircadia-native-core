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

#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtScript/QScriptContextInfo>

#include "../ScriptContext.h"

class QScriptContext;
class ScriptEngineQtScript;
class ScriptValue;
using ScriptValuePointer = QSharedPointer<ScriptValue>;

/// [QtScript] Implements ScriptContext for QtScript and translates calls for QScriptContextInfo
class ScriptContextQtWrapper : public ScriptContext {
public: // construction
    inline ScriptContextQtWrapper(ScriptEngineQtScript* engine, QScriptContext* context) : _engine(engine), _context(context) {}
    static ScriptContextQtWrapper* unwrap(ScriptContext* val);
    inline QScriptContext* toQtValue() const { return _context; }

public: // ScriptContext implementation
    virtual int argumentCount() const;
    virtual ScriptValuePointer argument(int index) const;
    virtual QStringList backtrace() const;
    virtual ScriptValuePointer callee() const;
    virtual ScriptEnginePointer engine() const;
    virtual ScriptFunctionContextPointer functionContext() const;
    virtual ScriptContextPointer parentContext() const;
    virtual ScriptValuePointer thisObject() const;
    virtual ScriptValuePointer throwError(const QString& text);
    virtual ScriptValuePointer throwValue(const ScriptValuePointer& value);

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
