//
//  SpeechRecognizer.mm
//  interface/src
//
//  Created by Ryan Huffman on 07/31/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGlobal>
#ifdef Q_OS_MAC

#import <Foundation/Foundation.h>
#import <AppKit/NSSpeechRecognizer.h>
#import <AppKit/NSWorkspace.h>

#include <QDebug>

#include "SpeechRecognizer.h"

@interface SpeechRecognizerDelegate : NSObject <NSSpeechRecognizerDelegate> {
    SpeechRecognizer* _listener;
}

- (void)setListener:(SpeechRecognizer*)listener;
- (void)speechRecognizer:(NSSpeechRecognizer*)sender didRecognizeCommand:(id)command;

@end

@implementation SpeechRecognizerDelegate

- (void)setListener:(SpeechRecognizer*)listener {
    _listener = listener;
}

- (void)speechRecognizer:(NSSpeechRecognizer*)sender didRecognizeCommand:(id)command {
    _listener->handleCommandRecognized(((NSString*)command).UTF8String);
}

@end

SpeechRecognizer::SpeechRecognizer() :
    QObject(),
    _enabled(false),
    _commands(),
    _speechRecognizerDelegate([[SpeechRecognizerDelegate alloc] init]),
    _speechRecognizer(NULL) {

    [(id)_speechRecognizerDelegate setListener:this];
}

SpeechRecognizer::~SpeechRecognizer() {
    if (_speechRecognizer) {
        [(id)_speechRecognizer dealloc];
    }
    if (_speechRecognizerDelegate) {
        [(id)_speechRecognizerDelegate dealloc];
    }
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
        _speechRecognizer = [[NSSpeechRecognizer alloc] init];

        reloadCommands();

        [(id)_speechRecognizer setDelegate:(id)_speechRecognizerDelegate];
        [(id)_speechRecognizer startListening];
    } else {
        [(id)_speechRecognizer stopListening];
        [(id)_speechRecognizer dealloc];
        _speechRecognizer = NULL;
    }

    emit enabledUpdated(_enabled);
}

void SpeechRecognizer::reloadCommands() {
    if (_speechRecognizer) {
        NSMutableArray* cmds = [NSMutableArray array];
        for (QSet<QString>::const_iterator iter = _commands.constBegin(); iter != _commands.constEnd(); iter++) {
            [cmds addObject:[NSString stringWithUTF8String:(*iter).toLocal8Bit().data()]];
        }
        [(id)_speechRecognizer setCommands:cmds];
    }
}

void SpeechRecognizer::addCommand(const QString& command) {
    _commands.insert(command);
    reloadCommands();
}

void SpeechRecognizer::removeCommand(const QString& command) {
    _commands.remove(command);
    reloadCommands();
}

#endif // Q_OS_MAC
