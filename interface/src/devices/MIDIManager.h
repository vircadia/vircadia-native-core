//
//  MIDIManager.h
//  interface/src/devices
//
//  Created by Stephen Birarda on 2014-06-30.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MIDIManager_h
#define hifi_MIDIManager_h

#include <QtCore/QObject>
#include <QtScript/QScriptEngine>

#include <MIDIEvent.h>

#ifdef HAVE_RTMIDI
#include <RtMidi.h>
#endif

class MIDIManager : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(unsigned int NoteOn READ NoteOn)
    Q_PROPERTY(unsigned int NoteOff READ NoteOff)
    Q_PROPERTY(unsigned int ModWheel READ ModWheel)
    Q_PROPERTY(unsigned int PitchWheel READ PitchWheel)
    
public:
    static MIDIManager& getInstance();
    static void midiCallback(double deltaTime, std::vector<unsigned char>* message, void* userData);
    
    ~MIDIManager();
    
    void openDefaultPort();
#ifdef HAVE_RTMIDI
    bool hasDevice() const { return !!_midiInput; }
#endif
public slots:
    unsigned int NoteOn() const { return 144; }
    unsigned int NoteOff() const { return 128; }
    unsigned int ModWheel() const  { return 176; }
    unsigned int PitchWheel() const { return 224; }
signals:
    void midiEvent(const MIDIEvent& event);
    
private:
#ifdef HAVE_RTMIDI
    RtMidiIn* _midiInput;
#endif
};


#endif // hifi_MIDIManager_h

