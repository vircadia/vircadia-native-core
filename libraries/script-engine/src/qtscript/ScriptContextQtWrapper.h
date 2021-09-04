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
class ScriptContextQtWrapper final : public ScriptContext {
public: // construction
    inline ScriptContextQtWrapper(ScriptEngineQtScript* engine, QScriptContext* context) : _context(context) , _engine(engine) {}
    static ScriptContextQtWrapper* unwrap(ScriptContext* val);
    inline QScriptContext* toQtValue() const { return _context; }

public: // ScriptContext implementation
    virtual int argumentCount() const override;
    virtual ScriptValue argument(int index) const override;
    virtual QStringList backtrace() const override;
    virtual ScriptValue callee() const override;
    virtual ScriptEnginePointer engine() const override;
    virtual ScriptFunctionContextPointer functionContext() const override;
    virtual ScriptContextPointer parentContext() const override;
    virtual ScriptValue thisObject() const override;
    virtual ScriptValue throwError(const QString& text) override;
    virtual ScriptValue throwValue(const ScriptValue& value) override;

private: // storage
    QScriptContext* _context;
    ScriptEngineQtScript* _engine;
};

class ScriptFunctionContextQtWrapper : public ScriptFunctionContext {
public:  // construction
    inline ScriptFunctionContextQtWrapper(QScriptContext* context) : _value(context) {}

public:  // ScriptFunctionContext implementation
    virtual QString fileName() const override;
    virtual QString functionName() const override;
    virtual FunctionType functionType() const override;
    virtual int lineNumber() const override;

private: // storage
    QScriptContextInfo _value;
};

#endif  // hifi_ScriptContextQtWrapper_h

/// @}
