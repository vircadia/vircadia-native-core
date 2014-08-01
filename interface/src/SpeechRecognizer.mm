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

#import <Foundation/Foundation.h>
#import <AppKit/NSSpeechRecognizer.h>

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
    _speechRecognizerDelegate(NULL),
    _speechRecognizer(NULL) {

    init();
}

SpeechRecognizer::~SpeechRecognizer() {
    if (_speechRecognizer) {
        [(id)_speechRecognizer dealloc];
    }
    if (_speechRecognizerDelegate) {
        [(id)_speechRecognizerDelegate dealloc];
    }
}

void SpeechRecognizer::init() {
    _speechRecognizerDelegate = [[SpeechRecognizerDelegate alloc] init];
    [(id)_speechRecognizerDelegate setListener:this];

    _speechRecognizer = [[NSSpeechRecognizer alloc] init];

    [(id)_speechRecognizer setCommands:[NSArray array]];
    [(id)_speechRecognizer setDelegate:(id)_speechRecognizerDelegate];

    setEnabled(_enabled);
}

void SpeechRecognizer::handleCommandRecognized(const char* command) {
    qDebug() << "Got command: " << command;
    emit commandRecognized(QString(command));
}

void SpeechRecognizer::setEnabled(bool enabled) {
    _enabled = enabled;
    if (enabled) {
        [(id)_speechRecognizer startListening];
    } else {
        [(id)_speechRecognizer stopListening];
    }
}

void SpeechRecognizer::addCommand(const QString& command) {
    NSArray *cmds = [(id)_speechRecognizer commands];
    NSString *cmd = [NSString stringWithUTF8String:command.toLocal8Bit().data()];
    [(id)_speechRecognizer setCommands:[cmds arrayByAddingObject:cmd]];
}

void SpeechRecognizer::removeCommand(const QString& command) {
    NSMutableArray* cmds = [NSMutableArray arrayWithArray:[(id)_speechRecognizer commands]];
    [cmds removeObject:[NSString stringWithUTF8String:command.toLocal8Bit().data()]];
    [(id)_speechRecognizer setCommands:cmds];
}
