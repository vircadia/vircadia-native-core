//
//  Midi.cpp
//  libraries/midi/src
//
//  Created by Burt Sloane
//  Modified by Bruce Brown
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Midi.h"


#include <QtCore/QLoggingCategory>

#if defined Q_OS_WIN32
#include "Windows.h"
#endif

#if defined Q_OS_WIN32
const int MIDI_BYTE_MASK = 0x0FF;
const int MIDI_NIBBLE_MASK = 0x00F;
const int MIDI_PITCH_BEND_MASK = 0x3F80;
const int MIDI_SHIFT_STATUS = 4;
const int MIDI_SHIFT_NOTE = 8;
const int MIDI_SHIFT_VELOCITY = 16;
const int MIDI_SHIFT_PITCH_BEND = 9;
//  Status Decode
const int MIDI_NOTE_OFF = 0x8;
const int MIDI_NOTE_ON = 0x9;
const int MIDI_POLYPHONIC_KEY_PRESSURE = 0xa;
const int MIDI_PROGRAM_CHANGE = 0xc;
const int MIDI_CHANNEL_PRESSURE = 0xd;
const int MIDI_PITCH_BEND_CHANGE = 0xe;
const int MIDI_SYSTEM_MESSAGE = 0xf;
#endif

const int MIDI_CONTROL_CHANGE = 0xb;
const int MIDI_CHANNEL_MODE_ALL_NOTES_OFF = 0x07b;

static Midi* instance = NULL;        // communicate this to non-class callbacks
static bool thruModeEnabled = false;
static bool broadcastEnabled = false;
static bool typeNoteOffEnabled = true;
static bool typeNoteOnEnabled = true;
static bool typePolyKeyPressureEnabled = false;
static bool typeControlChangeEnabled = true;
static bool typeProgramChangeEnabled = true;
static bool typeChanPressureEnabled = false;
static bool typePitchBendEnabled = true;
static bool typeSystemMessageEnabled = false;

std::vector<QString> Midi::midiInExclude;
std::vector<QString> Midi::midiOutExclude;

#if defined Q_OS_WIN32

#pragma comment(lib, "Winmm.lib")

//
std::vector<HMIDIIN> midihin;
std::vector<HMIDIOUT> midihout;

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    switch (wMsg) {
        case MIM_OPEN:
            // message not used
            break;
        case MIM_CLOSE:
            for (int i = 0; i < midihin.size(); i++) {
                if (midihin[i] == hMidiIn) {
                    midihin[i] = NULL;
                    instance->allNotesOff();
                    instance->midiHardwareChange();
                }
            }
            break;
        case MIM_DATA: {
            int device = -1;
            for (int i = 0; i < midihin.size(); i++) {
                if (midihin[i] == hMidiIn) {
                    device = i;
                }
            }
            int raw = dwParam1;
            int channel = (MIDI_NIBBLE_MASK & dwParam1) + 1;
            int status = MIDI_BYTE_MASK & dwParam1;
            int type = MIDI_NIBBLE_MASK & (dwParam1 >> MIDI_SHIFT_STATUS);
            int note = MIDI_BYTE_MASK & (dwParam1 >> MIDI_SHIFT_NOTE);
            int velocity = MIDI_BYTE_MASK & (dwParam1 >> MIDI_SHIFT_VELOCITY);
            int bend = 0;
            int program = 0;
            if (!typeNoteOffEnabled && type == MIDI_NOTE_OFF) {
                return;
            }
            if (!typeNoteOnEnabled && type == MIDI_NOTE_ON) {
                return;
            }
            if (!typePolyKeyPressureEnabled && type == MIDI_POLYPHONIC_KEY_PRESSURE) {
                return;
            }
            if (!typeControlChangeEnabled && type == MIDI_CONTROL_CHANGE) {
                return;
            }
            if (typeProgramChangeEnabled && type == MIDI_PROGRAM_CHANGE) {
                program = note;
                note = 0;
            }
            if (typeChanPressureEnabled && type == MIDI_CHANNEL_PRESSURE) {
                velocity = note;
                note = 0;
            }
            if (typePitchBendEnabled && type == MIDI_PITCH_BEND_CHANGE) {
                bend = ((MIDI_BYTE_MASK & (dwParam1 >> MIDI_SHIFT_NOTE)) | 
                    (MIDI_PITCH_BEND_MASK & (dwParam1 >> MIDI_SHIFT_PITCH_BEND))) - 8192;
                channel = 0;        // Weird values on different instruments
                note = 0;
                velocity = 0;
            }
            if (!typeSystemMessageEnabled && type == MIDI_SYSTEM_MESSAGE) {
                return;
            }
            if (thruModeEnabled) {
                instance->sendNote(status, note, velocity);        // relay the message on to all other midi devices.
            }
            instance->midiReceived(device, raw, channel, status, type, note, velocity, bend, program);        // notify the javascript
            break;
        }
    }
}

void CALLBACK MidiOutProc(HMIDIOUT hmo, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    switch (wMsg) {
        case MOM_OPEN:
            // message not used
            break;
        case MOM_CLOSE:
            for (int i = 0; i < midihout.size(); i++) {
                if (midihout[i] == hmo) {
                    midihout[i] = NULL;
                    instance->allNotesOff();
                    instance->midiHardwareChange();
                }
            }
            break;
    }
}

void Midi::sendRawMessage(int device, int raw) {
    if (broadcastEnabled) {
        for (int i = 0; i < midihout.size(); i++) {
            if (midihout[i] != NULL) {
                midiOutShortMsg(midihout[i], raw);
            }
        }
    } else {
        midiOutShortMsg(midihout[device], raw);
    }
}

void Midi::sendMessage(int device, int channel, int type, int note, int velocity) {
    int message = (channel - 1) | (type << MIDI_SHIFT_STATUS);
    if (broadcastEnabled) {
        for (int i = 1; i < midihout.size(); i++) {  //  Skip 0 (Microsoft GS Wavetable Synth)
            if (midihout[i] != NULL) {
                midiOutShortMsg(midihout[i], message | (note << MIDI_SHIFT_NOTE) | (velocity << MIDI_SHIFT_VELOCITY));
            }
        }
    } else {
        midiOutShortMsg(midihout[device], message | (note << MIDI_SHIFT_NOTE) | (velocity << MIDI_SHIFT_VELOCITY));
    }
}

void Midi::sendNote(int status, int note, int velocity) {
    for (int i = 1; i < midihout.size(); i++) {  //  Skip 0 (Microsoft GS Wavetable Synth)
        if (midihout[i] != NULL) {
            midiOutShortMsg(midihout[i], status | (note << MIDI_SHIFT_NOTE) | (velocity << MIDI_SHIFT_VELOCITY));
        }
    }
}

void Midi::MidiSetup() {
    midihin.clear();
    midihout.clear();

    MIDIINCAPS incaps;
    for (unsigned int i = 0; i < midiInGetNumDevs(); i++) {
        if (MMSYSERR_NOERROR == midiInGetDevCaps(i, &incaps, sizeof(MIDIINCAPS))) {

            bool found = false;
            for (int j = 0; j < midiInExclude.size(); j++) {
                if (midiInExclude[j].toStdString().compare(incaps.szPname) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {        // EXCLUDE AN INPUT BY NAME
                HMIDIIN tmphin;
                if (MMSYSERR_NOERROR == midiInOpen(&tmphin, i, (DWORD_PTR)MidiInProc, NULL, CALLBACK_FUNCTION)) {
                    if (MMSYSERR_NOERROR == midiInStart(tmphin)) {
                        midihin.push_back(tmphin);
                    }
                }
            }
        }
    }

    MIDIOUTCAPS outcaps;
    for (unsigned int i = 0; i < midiOutGetNumDevs(); i++) {
        if (MMSYSERR_NOERROR == midiOutGetDevCaps(i, &outcaps, sizeof(MIDIOUTCAPS))) {

            bool found = false;
            for (int j = 0; j < midiOutExclude.size(); j++) {
                if (midiOutExclude[j].toStdString().compare(outcaps.szPname) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {        // EXCLUDE AN OUTPUT BY NAME
                HMIDIOUT tmphout;
                if (MMSYSERR_NOERROR == midiOutOpen(&tmphout, i, (DWORD_PTR)MidiOutProc, NULL, CALLBACK_FUNCTION)) {
                    midihout.push_back(tmphout);
                }
            }
        }
    }

    allNotesOff();
}

void Midi::MidiCleanup() {
    allNotesOff();

    for (int i = 0; i < midihin.size(); i++) {
        if (midihin[i] != NULL) {
            midiInStop(midihin[i]);
            midiInClose(midihin[i]);
        }
    }
    for (int i = 0; i < midihout.size(); i++) {
        if (midihout[i] != NULL) {
            midiOutClose(midihout[i]);
        }
    }
    midihin.clear();
    midihout.clear();
}
#else
void Midi::sendRawMessage(int device, int raw) {
}

void Midi::sendNote(int status, int note, int velocity) {
}

void Midi::sendMessage(int device, int channel, int type, int note, int velocity){
}

void Midi::MidiSetup() {
    allNotesOff();
}

void Midi::MidiCleanup() {
    allNotesOff();
}
#endif

/*@jsdoc
 * A MIDI message.
 * <p><strong>Warning:</strong> The <code>status</code> property is NOT a MIDI status value.</p>
 * @typedef {object} Midi.MidiMessage
 * @property {number} device - Device number.
 * @property {Midi.RawMidiMessage} raw - Raw MIDI message.
 * @property {number} status - Channel + status. <em>Legacy value.</em>
 * @property {number} channel - Channel: <code>1</code> &ndash; <code>16</code>.
 * @property {number} type - Status: {@link Midi.MidiStatus}; <code>8</code> &ndash; <code>15</code>.
 * @property {number} note - Note: <code>0</code> &ndash; <code>127</code>.
 * @property {number} velocity - Note velocity: <code>0</code> &ndash; <code>127</code>. (<code>0</code> means "note off".)
 * @property {number} bend - Pitch bend: <code>-8192</code> &ndash; <code>8191</code>.
 * @property {number} program - Program change: <code>0</code> &ndash; <code>127</code>.
 */
/*@jsdoc
 * An integer DWORD (unsigned 32 bit) message with bits having values as follows:
 * <table>
 *   <tbody>
 *     <tr>
 *       <td width=25%><code>00000000</code></td>
 *       <td width=25%><code>0vvvvvvv</code></td>
 *       <td width=25%><code>0nnnnnnn</code></td>
 *       <td width=12%><code>1sss</code></td>
 *       <td width=12%><code>cccc</code></td>
 *   </tbody>
 * </table>
 * <p>Where:</p>
 * <ul>
 *   <li><code>v</code> = Velocity.
 *   <li><code>n</code> = Note.
 *   <li><code>s</code> = Status - {@link Midi.MidiStatus}
 *   <li><code>c</code> = Channel.
 * </ul>
 * <p>The number in the first bit of each byte denotes whether it is a command (1) or data (0).
 * @typedef {number} Midi.RawMidiMessage
 */
/*@jsdoc
 * <p>A MIDI status value. The following MIDI status values are supported:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>8</code></td><td>Note off.</td></tr>
 *     <tr><td><code>9</code></td><td>Note on.</td></tr>
 *     <tr><td><code>10</code></td><td>Polyphonic key pressure.</td></tr>
 *     <tr><td><code>11</code></td><td>Control change.</td></tr>
 *     <tr><td><code>12</code></td><td>Program change.</td></tr>
 *     <tr><td><code>13</code></td><td>Channel pressure.</td></tr>
 *     <tr><td><code>14</code></td><td>Pitch bend.</td></tr>
 *     <tr><td><code>15</code></td><td>System message.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} Midi.MidiStatus
 */
void Midi::midiReceived(int device, int raw, int channel, int status, int type, int note, int velocity, int bend, int program) {
    QVariantMap eventData;
    eventData["device"] = device;
    eventData["raw"] = raw;
    eventData["channel"] = channel;
    eventData["status"] = status;
    eventData["type"] = type;
    eventData["note"] = note;
    eventData["velocity"] = velocity;
    eventData["bend"] = bend;
    eventData["program"] = program;
    emit midiNote(eventData);// Legacy
    emit midiMessage(eventData);
}

void Midi::midiHardwareChange() {
    emit midiReset();
}
//

Midi::Midi() {
    instance = this;
    MidiSetup();
}

Midi::~Midi() {
}

void Midi::sendRawDword(int device, int raw) {
    sendRawMessage(device, raw);
}

void Midi::playMidiNote(int status, int note, int velocity) {
    sendNote(status, note, velocity);
}

void Midi::sendMidiMessage(int device, int channel, int type, int note, int velocity) {
    sendMessage(device, channel, type, note, velocity);
}

void Midi::allNotesOff() {
    sendNote(MIDI_CONTROL_CHANGE, MIDI_CHANNEL_MODE_ALL_NOTES_OFF, 0);        // all notes off
}

void Midi::resetDevices() {
    MidiCleanup();
    MidiSetup();
}

void Midi::USBchanged() {
    instance->MidiCleanup();
    instance->MidiSetup();
    instance->midiHardwareChange();
}

//

QStringList Midi::listMidiDevices(bool output) {
    QStringList rv;
#if defined Q_OS_WIN32
    if (output) {
        MIDIOUTCAPS outcaps;
        for (unsigned int i = 0; i < midiOutGetNumDevs(); i++) {
            midiOutGetDevCaps(i, &outcaps, sizeof(MIDIINCAPS));
            rv.append(outcaps.szPname);
        }
    } else {
        MIDIINCAPS incaps;
        for (unsigned int i = 0; i < midiInGetNumDevs(); i++) {
            midiInGetDevCaps(i, &incaps, sizeof(MIDIINCAPS));
            rv.append(incaps.szPname);
        }
    }
#endif
    return rv;
}

void Midi::unblockMidiDevice(QString name, bool output) {
    if (output) {
        for (unsigned long i = 0; i < midiOutExclude.size(); i++) {
            if (midiOutExclude[i].toStdString().compare(name.toStdString()) == 0) {
                midiOutExclude.erase(midiOutExclude.begin() + i);
                break;
            }
        }
    } else {
        for (unsigned long i = 0; i < midiInExclude.size(); i++) {
            if (midiInExclude[i].toStdString().compare(name.toStdString()) == 0) {
                midiInExclude.erase(midiInExclude.begin() + i);
                break;
            }
        }
    }
}

void Midi::blockMidiDevice(QString name, bool output) {
    unblockMidiDevice(name, output);        // make sure it's only in there once
    if (output) {
        midiOutExclude.push_back(name);
    } else {
        midiInExclude.push_back(name);
    }
}

void Midi::thruModeEnable(bool enable) {
    thruModeEnabled = enable;
}

void Midi::broadcastEnable(bool enable) {
    broadcastEnabled = enable;
}

void Midi::typeNoteOffEnable(bool enable) {
    typeNoteOffEnabled = enable;
}

void Midi::typeNoteOnEnable(bool enable) {
    typeNoteOnEnabled = enable;
}

void Midi::typePolyKeyPressureEnable(bool enable) {
    typePolyKeyPressureEnabled = enable;
}

void Midi::typeControlChangeEnable(bool enable) {
    typeControlChangeEnabled = enable;
}

void Midi::typeProgramChangeEnable(bool enable) {
    typeProgramChangeEnabled = enable;
}

void Midi::typeChanPressureEnable(bool enable) {
    typeChanPressureEnabled = enable;
}

void Midi::typePitchBendEnable(bool enable) {
    typePitchBendEnabled = enable;
}

void Midi::typeSystemMessageEnable(bool enable) {
    typeSystemMessageEnabled = enable;
}
