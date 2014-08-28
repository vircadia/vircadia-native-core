//
//  MIDIManager.cpp
//
//
//  Created by Stephen Birarda on 2014-06-30.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDebug>

#include "MIDIManager.h"

MIDIManager& MIDIManager::getInstance() {
    static MIDIManager sharedInstance;
    return sharedInstance;
}

void MIDIManager::midiCallback(double deltaTime, std::vector<unsigned char>* message, void* userData) {
#ifdef HAVE_RTMIDI
    
    MIDIEvent callbackEvent;
    callbackEvent.deltaTime = deltaTime;
    
    callbackEvent.type = message->at(0);
    
    if (message->size() > 1) {
        callbackEvent.data1 = message->at(1);
    }
    
    if (message->size() > 2) {
        callbackEvent.data2 = message->at(2);
    }
    
    emit getInstance().midiEvent(callbackEvent);
#endif
}

MIDIManager::~MIDIManager() {
#ifdef HAVE_RTMIDI
    delete _midiInput;
#endif
}

#ifdef HAVE_RTMIDI
const int DEFAULT_MIDI_PORT = 0;
#endif

void MIDIManager::openDefaultPort() {
#ifdef HAVE_RTMIDI
    if (!_midiInput) {
        _midiInput = new RtMidiIn();
        
        if (_midiInput->getPortCount() > 0) {
            qDebug() << "MIDIManager opening port" << DEFAULT_MIDI_PORT;
            
            _midiInput->openPort(DEFAULT_MIDI_PORT);
            
            // don't ignore sysex, timing, or active sensing messages
            _midiInput->ignoreTypes(false, false, false);
            
            _midiInput->setCallback(&MIDIManager::midiCallback);
        } else {
            qDebug() << "MIDIManager openDefaultPort called but there are no ports available.";
            delete _midiInput;
            _midiInput = NULL;
        }
    }
#endif
}