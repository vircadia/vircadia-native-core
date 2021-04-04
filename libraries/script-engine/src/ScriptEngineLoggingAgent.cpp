//
//  ScriptEngineLoggingAgent.cpp
//  script-engine/src
//
//  Created by Dale Glass on 04/04/2021.
//  Copyright 2021 Vircadia Contributors
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEngineLoggingAgent.h"
#include "ScriptContextHelper.h"
#include <QObject>
#include <QScriptEngine>
#include <QDebug>
#include <QLoggingCategory>
#include <QScriptContextInfo>

Q_LOGGING_CATEGORY(script_logger, "vircadia.script-engine.logging-agent")


ScriptEngineLoggingAgent::ScriptEngineLoggingAgent(QScriptEngine *engine) : QScriptEngineAgent(engine) {

}

void ScriptEngineLoggingAgent::functionEntry(qint64 scriptId) {
    if ( scriptId != -1) {
        // We only care about non-native functions
        return;
    }

    QStringList backtrace;
    QScriptContext *ctx = engine()->currentContext();
    while( ctx ) {
        QScriptContextInfo ctx_info(ctx);
        backtrace.append( QString("%1:%2 %3").arg(ctx_info.fileName()).arg(ctx_info.lineNumber()).arg(ctx_info.functionName()) );
        ctx = ctx->parentContext();
    }

    ScriptContextHelper::push( backtrace );
}

void ScriptEngineLoggingAgent::functionExit(qint64 scriptId, const QScriptValue &returnValue) {
    if ( scriptId != -1) {
        // We only care about non-native functions
        return;
    }

    ScriptContextHelper::pop();
}

