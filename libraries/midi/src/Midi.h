//
//  Midi.h
//  libraries/midi/src
//
//  Created by Burt Sloane
//  Modified by Bruce Brown
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
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

/*@jsdoc
 * The <code>Midi</code> API provides the ability to connect Interface with musical instruments and other external or virtual
 * devices via the MIDI protocol. For further information and examples, see the tutorial:
 * <a href="https://docs.vircadia.com/script/midi-tutorial.html">Use MIDI to Control Your Environment</a>.
 *
 * <p><strong>Note:</strong> Only works on Windows.</p>
 *
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

    /*@jsdoc
     * Triggered when a connected device sends an output.
     * @function Midi.midiNote
     * @param {Midi.MidiMessage} message - The MIDI message.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed. Use {@link Midi.midiMessage|midiMessage} instead.
     */
    void midiNote(QVariantMap eventData);

    /*@jsdoc
     * Triggered when a connected device sends an output.
     * @function Midi.midiMessage
     * @param {Midi.MidiMessage} message - The MIDI message.
     * @returns {Signal}
     */
    void midiMessage(QVariantMap eventData);

    /*@jsdoc
     * Triggered when the system detects there was a reset such as when a device is plugged in or unplugged.
     * @function Midi.midiReset
     * @returns {Signal}
     */
    void midiReset();

public slots:

    /*@jsdoc
     * Sends a raw MIDI packet to a particular device.
     * @function Midi.sendRawDword
     * @param {number} device - Integer device number.
     * @param {Midi.RawMidiMessage} raw - Raw MIDI message.
     */
    Q_INVOKABLE void sendRawDword(int device, int raw);

    /*@jsdoc
     * Sends a MIDI message to a particular device.
     * @function Midi.sendMidiMessage
     * @param {number} device - Integer device number.
     * @param {number} channel - Integer channel number.
     * @param {Midi.MidiStatus} type - Integer status value.
     * @param {number} note - Note number.
     * @param {number} velocity - Note velocity. (<code>0</code> means "note off".)
     * @comment The "type" parameter has that name to match up with {@link Midi.MidiMessage}.
     */
    Q_INVOKABLE void sendMidiMessage(int device, int channel, int type, int note, int velocity);

    /*@jsdoc
     * Plays a note on all connected devices.
     * @function Midi.playMidiNote
     * @param {MidiStatus} status - Note status.
     * @param {number} note - Note number.
     * @param {number} velocity - Note velocity. (<code>0</code> means "note off".)
     */
    Q_INVOKABLE void playMidiNote(int status, int note, int velocity);

    /*@jsdoc
     * Turns off all notes on all connected MIDI devices.
     * @function Midi.allNotesOff
     */
    Q_INVOKABLE void allNotesOff();

    /*@jsdoc
     * Cleans up and rediscovers attached MIDI devices.
     * @function Midi.resetDevices
     */
    Q_INVOKABLE void resetDevices();

    /*@jsdoc
     * Gets a list of MIDI input or output devices.
     * @function Midi.listMidiDevices
     * @param {boolean} output - <code>true</code> to list output devices, <code>false</code> to list input devices.
     * @returns {string[]}
     */
    Q_INVOKABLE QStringList listMidiDevices(bool output);

    /*@jsdoc
     * Blocks a MIDI device's input or output.
     * @function Midi.blockMidiDevice
     * @param {string} name - The name of the MIDI device to block.
     * @param {boolean} output -  <code>true</code> to block the device's output, <code>false</code> to block its input.
     */
    Q_INVOKABLE void blockMidiDevice(QString name, bool output);

    /*@jsdoc
     * Unblocks a MIDI device's input or output.
     * @function Midi.unblockMidiDevice
     * @param {string} name- The name of the MIDI device to unblock.
     * @param {boolean} output -  <code>true</code> to unblock the device's output, <code>false</code> to unblock its input.
     */
    Q_INVOKABLE void unblockMidiDevice(QString name, bool output);

    /*@jsdoc
     * Enables or disables repeating all incoming notes to all outputs. (Default is disabled.)
     * @function Midi.thruModeEnable
     * @param {boolean} enable - <code>true</code> to enable repeating all incoming notes to all output, <code>false</code> to
     *     disable.
     */
    Q_INVOKABLE void thruModeEnable(bool enable);


    /*@jsdoc
     * Enables or disables broadcasts to all unblocked devices.
     * @function Midi.broadcastEnable
     * @param {boolean} enable - <code>true</code> to have "send" functions broadcast to all devices, <code>false</code> to
     *     have them send to specific output devices.
     */
    Q_INVOKABLE void broadcastEnable(bool enable);


    /// filter by event types

    /*@jsdoc
     * Enables or disables note off events.
     * @function Midi.typeNoteOffEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typeNoteOffEnable(bool enable);

    /*@jsdoc
     * Enables or disables note on events.
     * @function Midi.typeNoteOnEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typeNoteOnEnable(bool enable);

    /*@jsdoc
     * Enables or disables poly key pressure events.
     * @function Midi.typePolyKeyPressureEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typePolyKeyPressureEnable(bool enable);

    /*@jsdoc
     * Enables or disables control change events.
     * @function Midi.typeControlChangeEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typeControlChangeEnable(bool enable);

    /*@jsdoc
     * Enables or disables program change events.
     * @function Midi.typeProgramChangeEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typeProgramChangeEnable(bool enable);

    /*@jsdoc
     * Enables or disables channel pressure events.
     * @function Midi.typeChanPressureEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typeChanPressureEnable(bool enable);

    /*@jsdoc
     * Enables or disables pitch bend events.
     * @function Midi.typePitchBendEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typePitchBendEnable(bool enable);

    /*@jsdoc
     * Enables or disables system message events.
     * @function Midi.typeSystemMessageEnable
     * @param {boolean} enable - <code>true</code> to enable, <code>false</code> to disable.
     */
    Q_INVOKABLE void typeSystemMessageEnable(bool enable);


public:
    Midi();
    virtual ~Midi();
};

#endif // hifi_Midi_h
