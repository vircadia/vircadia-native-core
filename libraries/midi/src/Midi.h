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

/**jsdoc
 * @namespace Midi
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */

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

    /**jsdoc
     * Send Raw MIDI packet to a particular device.
     * @function Midi.sendRawDword
     * @param {number} device - Integer device number.
     * @param {number} raw - Integer (DWORD) raw MIDI message.
     */
    Q_INVOKABLE void sendRawDword(int device, int raw);

    /**jsdoc
     * Send MIDI message to a particular device.
     * @function Midi.sendMidiMessage
     * @param {number} device - Integer device number.
     * @param {number} channel - Integer channel number.
     * @param {number} type - 0x8 is note off, 0x9 is note on (if velocity=0, note off), etc.
     * @param {number} note - MIDI note number.
     * @param {number} velocity - Note velocity (0 means note off).
     */
    Q_INVOKABLE void sendMidiMessage(int device, int channel, int type, int note, int velocity);

    /**jsdoc
     * Play a note on all connected devices.
     * @function Midi.playMidiNote
     * @param {number} status - 0x80 is note off, 0x90 is note on (if velocity=0, note off), etc.
     * @param {number} note - MIDI note number.
     * @param {number} velocity - Note velocity (0 means note off).
     */
    Q_INVOKABLE void playMidiNote(int status, int note, int velocity);

    /**jsdoc
     * Turn off all notes on all connected devices.
     * @function Midi.allNotesOff
     */
    Q_INVOKABLE void allNotesOff();

    /**jsdoc
     * Clean up and re-discover attached devices.
     * @function Midi.resetDevices
     */
    Q_INVOKABLE void resetDevices();

    /**jsdoc
     * Get a list of inputs/outputs.
     * @function Midi.listMidiDevices
     * @param {boolean} output
     * @returns {string[]}
     */
    Q_INVOKABLE QStringList listMidiDevices(bool output);

    /**jsdoc
     * Block an input/output by name.
     * @function Midi.blockMidiDevice
     * @param {string} name
     * @param {boolean} output
     */
    Q_INVOKABLE void blockMidiDevice(QString name, bool output);

    /**jsdoc
     * Unblock an input/output by name.
     * @function Midi.unblockMidiDevice
     * @param {string} name
     * @param {boolean} output
     */
    Q_INVOKABLE void unblockMidiDevice(QString name, bool output);

    /**jsdoc
     * Repeat all incoming notes to all outputs (default disabled).
     * @function Midi.thruModeEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void thruModeEnable(bool enable);


    /**jsdoc
     * Broadcast on all unblocked devices.
     * @function Midi.broadcastEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void broadcastEnable(bool enable);
    

    /// filter by event types

    /**jsdoc
     * @function Midi.typeNoteOffEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typeNoteOffEnable(bool enable);

    /**jsdoc
     * @function Midi.typeNoteOnEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typeNoteOnEnable(bool enable);

    /**jsdoc
     * @function Midi.typePolyKeyPressureEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typePolyKeyPressureEnable(bool enable);

    /**jsdoc
     * @function Midi.typeControlChangeEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typeControlChangeEnable(bool enable);

    /**jsdoc
     * @function Midi.typeProgramChangeEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typeProgramChangeEnable(bool enable);

    /**jsdoc
     * @function Midi.typeChanPressureEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typeChanPressureEnable(bool enable);

    /**jsdoc
     * @function Midi.typePitchBendEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typePitchBendEnable(bool enable);

    /**jsdoc
     * @function Midi.typeSystemMessageEnable
     * @param {boolean} enable
     */
    Q_INVOKABLE void typeSystemMessageEnable(bool enable);


public:
    Midi();
    virtual ~Midi();
};

#endif // hifi_Midi_h
