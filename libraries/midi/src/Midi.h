//
//  Midi.h
//  libraries/midi/src
//
//  Created by Burt Sloane
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Midi_h
#define hifi_Midi_h

#include <QtCore/QObject>
#include <QAbstractNativeEventFilter>
#include <DependencyManager.h>

#include <vector>
#include <string>

class Midi : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void noteReceived(int status, int note, int velocity);    // relay a note to Javascript
    void sendNote(int status, int note, int vel);            // relay a note to MIDI outputs
    static void USBchanged();

private:
    static std::vector<QString> midiinexclude;
    static std::vector<QString> midioutexclude;

private:
    void MidiSetup();
    void MidiCleanup();

signals:
    void midiNote(QVariantMap eventData);

public slots:
/// play a note on all connected devices
/// @param {int} status: 0x80 is noteoff, 0x90 is noteon (if velocity=0, noteoff), etc
/// @param {int} note: midi note number
/// @param {int} velocity: note velocity (0 means noteoff)
Q_INVOKABLE void playMidiNote(int status, int note, int velocity);

/// turn off all notes on all connected devices
Q_INVOKABLE void allNotesOff();

/// clean up and re-discover attached devices
Q_INVOKABLE void resetDevices();

/// ask for a list of inputs/outputs
Q_INVOKABLE QStringList listMidiDevices(bool output);

/// block an input/output by name
Q_INVOKABLE void blockMidiDevice(QString name, bool output);

/// unblock an input/output by name
Q_INVOKABLE void unblockMidiDevice(QString name, bool output);

/// repeat all incoming notes to all outputs (default disabled)
Q_INVOKABLE void thruModeEnable(bool enable);

public:
    Midi();
	virtual ~Midi();
};

#endif // hifi_Midi_h
