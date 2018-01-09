//
//  Midi.h
//  libraries/midi/src
//
//  Created by Burt Sloane
//  Modified by Bruce Brown
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
    void midiReceived(int device, int raw, int channel, int status, int type, int note, int velocity, int bend, int program);  // relay a note to Javascript
    void midiHardwareChange();  // relay hardware change to Javascript
    void sendRawMessage(int device, int raw);  // relay midi message to MIDI outputs
    void sendNote(int status, int note, int velocity);  // relay a note to MIDI outputs
    void sendMessage(int device, int channel, int type, int note, int velocity);  // relay a message to MIDI outputs 
    static void USBchanged();

private:
    static std::vector<QString> midiInExclude;
    static std::vector<QString> midiOutExclude;

private:
    void MidiSetup();
    void MidiCleanup();

signals:
    void midiNote(QVariantMap eventData);
    void midiMessage(QVariantMap eventData);
    void midiReset();

    public slots:
    // Send Raw Midi Packet to all connected devices
    Q_INVOKABLE void sendRawDword(int device, int raw);
    /// Send Raw Midi message to selected device
    /// @param {int} device: device number
    /// @param {int} raw: raw midi message (DWORD)

    // Send Midi Message to all connected devices 
    Q_INVOKABLE void sendMidiMessage(int device, int channel, int type, int note, int velocity);
    /// Send midi message to selected device/devices
    /// @param {int} device: device number
    /// @param {int} channel: channel number
    /// @param {int} type: 0x8 is noteoff, 0x9 is noteon (if velocity=0, noteoff), etc
    /// @param {int} note: midi note number
    /// @param {int} velocity: note velocity (0 means noteoff)

    // Send Midi Message to all connected devices 
    Q_INVOKABLE void playMidiNote(int status, int note, int velocity);
    /// play a note on all connected devices
    /// @param {int} status: 0x80 is noteoff, 0x90 is noteon (if velocity=0, noteoff), etc
    /// @param {int} note: midi note number
    /// @param {int} velocity: note velocity (0 means noteoff)

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

    /// broadcast on all unblocked devices
    Q_INVOKABLE void broadcastEnable(bool enable);
    
    /// filter by event types
    Q_INVOKABLE void typeNoteOffEnable(bool enable);
    Q_INVOKABLE void typeNoteOnEnable(bool enable);
    Q_INVOKABLE void typePolyKeyPressureEnable(bool enable);
    Q_INVOKABLE void typeControlChangeEnable(bool enable);
    Q_INVOKABLE void typeProgramChangeEnable(bool enable);
    Q_INVOKABLE void typeChanPressureEnable(bool enable);
    Q_INVOKABLE void typePitchBendEnable(bool enable);
    Q_INVOKABLE void typeSystemMessageEnable(bool enable);


public:
    Midi();
    virtual ~Midi();
};

#endif // hifi_Midi_h
