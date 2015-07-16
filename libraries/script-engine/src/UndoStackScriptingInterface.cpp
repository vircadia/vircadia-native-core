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
#include <QScriptValue>
#include <QScriptValueList>
#include <QScriptEngine>

#include "UndoStackScriptingInterface.h"

UndoStackScriptingInterface::UndoStackScriptingInterface(QUndoStack* undoStack) : _undoStack(undoStack) {
}

void UndoStackScriptingInterface::pushCommand(QScriptValue undoFunction, QScriptValue undoData,
                                              QScriptValue redoFunction, QScriptValue redoData) {
    if (undoFunction.engine()) {
        ScriptUndoCommand* undoCommand = new ScriptUndoCommand(undoFunction, undoData, redoFunction, redoData);
        undoCommand->moveToThread(undoFunction.engine()->thread());
        _undoStack->push(undoCommand);
    }
}

ScriptUndoCommand::ScriptUndoCommand(QScriptValue undoFunction, QScriptValue undoData,
                                     QScriptValue redoFunction, QScriptValue redoData) :
    _hasRedone(false),
    _undoFunction(undoFunction),
    _undoData(undoData),
    _redoFunction(redoFunction),
    _redoData(redoData) {
}

void ScriptUndoCommand::undo() {
    QMetaObject::invokeMethod(this, "doUndo");
}

void ScriptUndoCommand::redo() {
    // QUndoStack will call `redo()` when adding a command to the stack.  This
    // makes it difficult to work with commands that span a period of time - for instance,
    // the entity duplicate + move command that duplicates an entity and then moves it.
    // A better implementation might be to properly implement `mergeWith()` and `id()`
    // so that the two actions in the example would be merged.
    if (_hasRedone) {
        QMetaObject::invokeMethod(this, "doRedo");
    }
    _hasRedone = true;
}

void ScriptUndoCommand::doUndo() {
    QScriptValueList args;
    args << _undoData;
    _undoFunction.call(QScriptValue(), args);
}

void ScriptUndoCommand::doRedo() {
    QScriptValueList args;
    args << _redoData;
    _redoFunction.call(QScriptValue(), args);
}
