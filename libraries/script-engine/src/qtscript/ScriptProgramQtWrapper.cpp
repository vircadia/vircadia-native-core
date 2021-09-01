//
//  ScriptProgramQtWrapper.cpp
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 8/24/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptProgramQtWrapper.h"

#include <QtScript/QScriptContext>

#include "ScriptEngineQtScript.h"
#include "ScriptValueQtWrapper.h"

ScriptProgramQtWrapper* ScriptProgramQtWrapper::unwrap(ScriptProgramPointer val) {
    if (!val) {
        return nullptr;
    }

    return dynamic_cast<ScriptProgramQtWrapper*>(val.get());
}

ScriptSyntaxCheckResultPointer ScriptProgramQtWrapper::checkSyntax() const {
    QScriptSyntaxCheckResult result = _engine->checkSyntax(_value.sourceCode());
    return ScriptSyntaxCheckResultPointer(new ScriptSyntaxCheckResultQtWrapper(std::move(result)));
}

QString ScriptProgramQtWrapper::fileName() const {
    return _value.fileName();
}

QString ScriptProgramQtWrapper::sourceCode() const {
    return _value.sourceCode();
}


int ScriptSyntaxCheckResultQtWrapper::errorColumnNumber() const {
    return _value.errorColumnNumber();
}

int ScriptSyntaxCheckResultQtWrapper::errorLineNumber() const {
    return _value.errorLineNumber();
}

QString ScriptSyntaxCheckResultQtWrapper::errorMessage() const {
    return _value.errorMessage();
}

ScriptSyntaxCheckResult::State ScriptSyntaxCheckResultQtWrapper::state() const {
    return static_cast<ScriptSyntaxCheckResult::State>(_value.state());
}
