//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "JSEndpoint.h"
#include "../../Logging.h"

using namespace controller;

QString formatException(const QJSValue& exception) {
    QString note { "UncaughtException" };
    QString result;

    const auto message = exception.toString();
    const auto fileName = exception.property("fileName").toString();
    const auto lineNumber = exception.property("lineNumber").toString();
    const auto stacktrace = exception.property("stack").toString();

    const QString SCRIPT_EXCEPTION_FORMAT = "[%0] %1 in %2:%3";
    const QString SCRIPT_BACKTRACE_SEP = "\n    ";

    result = QString(SCRIPT_EXCEPTION_FORMAT).arg(note, message, fileName, lineNumber);
    if (!stacktrace.isEmpty()) {
        result += QString("\n[Backtrace]%1%2").arg(SCRIPT_BACKTRACE_SEP).arg(stacktrace);
    }
    return result;
}

float JSEndpoint::peek() const {
    QJSValue result = _callable.call();
    if (result.isError()) {
        qCDebug(controllers).noquote() << formatException(result);
        return 0.0f;
    } else {
        return (float)result.toNumber();
    }
}

void JSEndpoint::apply(float newValue, const Pointer& source) {
    QJSValue result = _callable.call(QJSValueList({ QJSValue(newValue) }));
    if (result.isError()) {
        qCDebug(controllers).noquote() << formatException(result);
    }
}
