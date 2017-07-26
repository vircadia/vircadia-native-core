//
//  Midi.cpp
//  libraries/midi/src
//
//  Created by Burt Sloane
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


const int MIDI_BYTE_MASK = 0x0FF;
const int MIDI_SHIFT_NOTE = 8;
const int MIDI_SHIFT_VELOCITY = 16;
const int MIDI_STATUS_MASK = 0x0F0;
const int MIDI_NOTE_OFF = 0x080;
const int MIDI_NOTE_ON = 0x090;
const int MIDI_CONTROL_CHANGE = 0x0b0;
const int MIDI_CHANNEL_MODE_ALL_NOTES_OFF = 0x07b;


static Midi* instance = NULL;            // communicate this to non-class callbacks
static bool thruModeEnabled = false;

std::vector<QString> Midi::midiinexclude;
std::vector<QString> Midi::midioutexclude;


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
                }
            }
            break;
        case MIM_DATA: {
            int status = MIDI_BYTE_MASK & dwParam1;
            int note = MIDI_BYTE_MASK & (dwParam1 >> MIDI_SHIFT_NOTE);
            int vel = MIDI_BYTE_MASK & (dwParam1 >> MIDI_SHIFT_VELOCITY);
            if (thruModeEnabled) {
                instance->sendNote(status, note, vel);          // relay the note on to all other midi devices
            }
            instance->noteReceived(status, note, vel);          // notify the javascript
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
                }
            }
            break;
    }
}


void Midi::sendNote(int status, int note, int vel) {
    for (int i = 0; i < midihout.size(); i++) {
        if (midihout[i] != NULL) {
            midiOutShortMsg(midihout[i], status + (note << MIDI_SHIFT_NOTE) + (vel << MIDI_SHIFT_VELOCITY));
        }
    }
}


void Midi::MidiSetup() {
    midihin.clear();
    midihout.clear();

    MIDIINCAPS incaps;
    for (unsigned int i = 0; i < midiInGetNumDevs(); i++) {
        midiInGetDevCaps(i, &incaps, sizeof(MIDIINCAPS));

        bool found = false;
        for (int j = 0; j < midiinexclude.size(); j++) {
            if (midiinexclude[j].toStdString().compare(incaps.szPname) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {        // EXCLUDE AN INPUT BY NAME
            HMIDIIN tmphin;
            midiInOpen(&tmphin, i, (DWORD_PTR)MidiInProc, NULL, CALLBACK_FUNCTION);
            midiInStart(tmphin);
            midihin.push_back(tmphin);
        }

    }

    MIDIOUTCAPS outcaps;
    for (unsigned int i = 0; i < midiOutGetNumDevs(); i++) {
        midiOutGetDevCaps(i, &outcaps, sizeof(MIDIINCAPS));

        bool found = false;
        for (int j = 0; j < midioutexclude.size(); j++) {
            if (midioutexclude[j].toStdString().compare(outcaps.szPname) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {        // EXCLUDE AN OUTPUT BY NAME
            HMIDIOUT tmphout;
            midiOutOpen(&tmphout, i, (DWORD_PTR)MidiOutProc, NULL, CALLBACK_FUNCTION);
            midihout.push_back(tmphout);
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
void Midi::sendNote(int status, int note, int vel) {
}

void Midi::MidiSetup() {
    allNotesOff();
}

void Midi::MidiCleanup() {
    allNotesOff();
}
#endif

void Midi::noteReceived(int status, int note, int velocity) {
    if (((status & MIDI_STATUS_MASK) != MIDI_NOTE_OFF) &&
        ((status & MIDI_STATUS_MASK) != MIDI_NOTE_ON)) {
        return;            // NOTE: only sending note-on and note-off to Javascript
    }

    QVariantMap eventData;
    eventData["status"] = status;
    eventData["note"] = note;
    eventData["velocity"] = velocity;
//    emit midiNote(eventData);
}

//

Midi::Midi() {
    instance = this;
#if defined Q_OS_WIN32
    midioutexclude.push_back("Microsoft GS Wavetable Synth");        // we don't want to hear this thing
#endif
    MidiSetup();
}

Midi::~Midi() {
}

void Midi::playMidiNote(int status, int note, int velocity) {
    sendNote(status, note, velocity);
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
        for (unsigned long i = 0; i < midioutexclude.size(); i++) {
            if (midioutexclude[i].toStdString().compare(name.toStdString()) == 0) {
                midioutexclude.erase(midioutexclude.begin() + i);
                break;
            }
        }
    } else {
        for (unsigned long i = 0; i < midiinexclude.size(); i++) {
            if (midiinexclude[i].toStdString().compare(name.toStdString()) == 0) {
                midiinexclude.erase(midiinexclude.begin() + i);
                break;
            }
        }
    }
}

void Midi::blockMidiDevice(QString name, bool output) {
    unblockMidiDevice(name, output);        // make sure it's only in there once
    if (output) {
        midioutexclude.push_back(name);
    } else {
        midiinexclude.push_back(name);
    }
}

void Midi::thruModeEnable(bool enable) {
    thruModeEnabled = enable;
}

/*
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ -arch x86_64 -isysroot
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk -L/Users/burt/hifi/assignment-client/Debug -F/Users/burt/hifi/assignment-client/Debug -F/Users/burt/Qt5.6.2/5.6/clang_64/lib -filelist
/Users/burt/hifi/assignment-client/hifi.build/Debug/assignment-client.build/Objects-normal/x86_64/assignment-client.LinkFileList -Xlinker -rpath -Xlinker
/Users/burt/Qt5.6.2/5.6/clang_64/lib -mmacosx-version-min=10.8 -Xlinker -object_path_lto -Xlinker
/Users/burt/hifi/assignment-client/hifi.build/Debug/assignment-client.build/Objects-normal/x86_64/assignment-client_lto.o -Xlinker -no_deduplicate -stdlib=libc++ -Wl,-search_paths_first -Wl,-headerpad_max_install_names
/Users/burt/hifi/libraries/audio/Debug/libaudio.a
/Users/burt/hifi/libraries/avatars/Debug/libavatars.a
/Users/burt/hifi/libraries/octree/Debug/liboctree.a
/Users/burt/hifi/libraries/gpu/Debug/libgpu.a
/Users/burt/hifi/libraries/model/Debug/libmodel.a
/Users/burt/hifi/libraries/fbx/Debug/libfbx.a
/Users/burt/hifi/libraries/entities/Debug/libentities.a
/Users/burt/hifi/libraries/networking/Debug/libnetworking.a
/Users/burt/hifi/libraries/animation/Debug/libanimation.a
/Users/burt/hifi/libraries/recording/Debug/librecording.a
/Users/burt/hifi/libraries/shared/Debug/libshared.a
/Users/burt/hifi/libraries/script-engine/Debug/libscript-engine.a
/Users/burt/hifi/libraries/embedded-webserver/Debug/libembedded-webserver.a
/Users/burt/hifi/libraries/controllers/Debug/libcontrollers.a
/Users/burt/hifi/libraries/physics/Debug/libphysics.a
/Users/burt/hifi/libraries/plugins/Debug/libplugins.a
/Users/burt/hifi/libraries/ui/Debug/libui.a
/Users/burt/hifi/libraries/script-engine/Debug/libscript-engine.a
/Users/burt/hifi/libraries/ui/Debug/libui.a
/Users/burt/hifi/libraries/recording/Debug/librecording.a
/Users/burt/hifi/libraries/controllers/Debug/libcontrollers.a
/Users/burt/hifi/libraries/physics/Debug/libphysics.a
/Users/burt/hifi/libraries/entities/Debug/libentities.a
/Users/burt/hifi/libraries/audio/Debug/libaudio.a
/Users/burt/hifi/libraries/plugins/Debug/libplugins.a
/Users/burt/hifi/libraries/avatars/Debug/libavatars.a
/Users/burt/hifi/libraries/octree/Debug/liboctree.a
/Users/burt/hifi/libraries/animation/Debug/libanimation.a
/Users/burt/hifi/ext/Xcode/bullet/project/lib/libBulletDynamics.dylib
/Users/burt/hifi/ext/Xcode/bullet/project/lib/libBulletCollision.dylib
/Users/burt/hifi/ext/Xcode/bullet/project/lib/libLinearMath.dylib
/Users/burt/hifi/ext/Xcode/bullet/project/lib/libBulletSoftBody.dylib
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtScriptTools.framework/QtScriptTools
/Users/burt/hifi/ext/Xcode/quazip/project/lib/libquazip5d.1.0.0.dylib
/Users/burt/hifi/libraries/procedural/Debug/libprocedural.a
/Users/burt/hifi/libraries/model-networking/Debug/libmodel-networking.a
/Users/burt/hifi/libraries/fbx/Debug/libfbx.a
/Users/burt/hifi/libraries/model/Debug/libmodel.a
/Users/burt/hifi/libraries/image/Debug/libimage.a
/Users/burt/hifi/ext/Xcode/nvtt/project/lib/libnvtt.dylib
/Users/burt/hifi/libraries/gpu-gl/Debug/libgpu-gl.a
/Users/burt/hifi/libraries/gpu/Debug/libgpu.a
/Users/burt/hifi/libraries/ktx/Debug/libktx.a
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtConcurrent.framework/QtConcurrent -lpthread
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtWebSockets.framework/QtWebSockets
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtXmlPatterns.framework/QtXmlPatterns
/Users/burt/hifi/libraries/gl/Debug/libgl.a
/Users/burt/hifi/libraries/networking/Debug/libnetworking.a
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtWebEngine.framework/QtWebEngine
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtWebEngineCore.framework/QtWebEngineCore
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtQuick.framework/QtQuick
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtWebChannel.framework/QtWebChannel
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtPositioning.framework/QtPositioning
/usr/local/Cellar/openssl/1.0.2k/lib/libssl.dylib
/usr/local/Cellar/openssl/1.0.2k/lib/libcrypto.dylib
/Users/burt/hifi/ext/Xcode/tbb/project/src/tbb/lib/libtbb_debug.dylib
/Users/burt/hifi/ext/Xcode/tbb/project/src/tbb/lib/libtbbmalloc_debug.dylib -framework IOKit -framework CoreFoundation
/Users/burt/hifi/libraries/shared/Debug/libshared.a
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtScript.framework/QtScript
/usr/local/lib/libz.a
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtQml.framework/QtQml
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtOpenGL.framework/QtOpenGL
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtWidgets.framework/QtWidgets
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtGui.framework/QtGui -framework OpenGL
/Users/burt/hifi/ext/Xcode/glew/project/lib/libglew_d.a
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtNetwork.framework/QtNetwork
/Users/burt/Qt5.6.2/5.6/clang_64/lib/QtCore.framework/QtCore -Xlinker -dependency_info -Xlinker
/Users/burt/hifi/assignment-client/hifi.build/Debug/assignment-client.build/Objects-normal/x86_64/assignment-client_dependency_info.dat -o
/Users/burt/hifi/assignment-client/Debug/assignment-client
*/

