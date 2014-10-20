//
//  SpeechRecognizer.cpp
//  interface/src
//
//  Created by David Rowe on 10/20/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGlobal>

#ifdef Q_OS_WIN

#include "SpeechRecognizer.h"

SpeechRecognizer::SpeechRecognizer() :
    QObject(),
    _enabled(false),
    _commands() {

}

SpeechRecognizer::~SpeechRecognizer() {

}

void SpeechRecognizer::handleCommandRecognized(const char* command) {
    emit commandRecognized(QString(command));
}

void SpeechRecognizer::setEnabled(bool enabled) {
    if (enabled == _enabled) {
        return;
    }

    _enabled = enabled;
    if (_enabled) {

    } else {

    }

    emit enabledUpdated(_enabled);
}

void SpeechRecognizer::addCommand(const QString& command) {
    _commands.insert(command);
    reloadCommands();
}

void SpeechRecognizer::removeCommand(const QString& command) {
    _commands.remove(command);
    reloadCommands();
}

void SpeechRecognizer::reloadCommands() {

}

#endif // Q_OS_WIN