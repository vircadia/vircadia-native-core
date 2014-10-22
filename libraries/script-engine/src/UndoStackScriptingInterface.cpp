//
//  UndoStackScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Ryan Huffman on 10/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QScriptValueList>
#include <QScriptEngine>

#include "UndoStackScriptingInterface.h"

UndoStackScriptingInterface::UndoStackScriptingInterface(QUndoStack* undoStack) : _undoStack(undoStack) {
}

void UndoStackScriptingInterface::pushCommand(QScriptValue undoFunction, QScriptValue undoData,
                                              QScriptValue redoFunction, QScriptValue redoData) {
    ScriptUndoCommand* undoCommand = new ScriptUndoCommand(undoFunction, undoData, redoFunction, redoData);
    qDebug() << "Pushing command";
    _undoStack->push(undoCommand);
}

ScriptUndoCommand::ScriptUndoCommand(QScriptValue undoFunction, QScriptValue undoData,
                                     QScriptValue redoFunction, QScriptValue redoData) :
    _undoFunction(undoFunction),
    _undoData(undoData),
    _redoFunction(redoFunction),
    _redoData(redoData) {
}

void ScriptUndoCommand::undo() {
    QScriptValueList args;
    args << _undoData;
    _undoFunction.call(QScriptValue(), args);
}

void ScriptUndoCommand::redo() {
    QScriptValueList args;
    args << _redoData;
    _redoFunction.call(QScriptValue(), args);
}
