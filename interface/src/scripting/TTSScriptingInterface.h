//  TTSScriptingInterface.h
//  libraries/audio-client/src/scripting
//
//  Created by Zach Fox on 2018-10-10.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpeechScriptingInterface_h
#define hifi_SpeechScriptingInterface_h

#include <QtCore/QObject>
#include <DependencyManager.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <sapi.h>      // SAPI
#include <sphelper.h>  // SAPI Helper
#include <AudioInjector.h>
#include <AudioConstants.h>

class TTSScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    TTSScriptingInterface();
    ~TTSScriptingInterface();

    Q_INVOKABLE void testTone(const bool& alsoInject = false);
    Q_INVOKABLE void speakText(const QString& textToSpeak,
                               const int& newChunkSize = (AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * 50),
                               const int& timerInterval = 96,
                               const int& sampleRate = 24000,
                               const int& bitsPerSample = 16,
                               const bool& alsoInject = false);
    Q_INVOKABLE void stopLastSpeech();

signals:
    void ttsSampleCreated(QByteArray outputArray, const int& newChunkSize, const int& timerInterval);
    void clearTTSBuffer();

private:
    class CComAutoInit {
    public:
        // Initializes COM using CoInitialize.
        // On failure, signals error using AtlThrow.
        CComAutoInit() {
            HRESULT hr = ::CoInitialize(NULL);
            if (FAILED(hr)) {
                ATLTRACE(TEXT("CoInitialize() failed in CComAutoInit constructor (hr=0x%08X).\n"), hr);
                AtlThrow(hr);
            }
        }

        // Initializes COM using CoInitializeEx.
        // On failure, signals error using AtlThrow.
        explicit CComAutoInit(__in DWORD dwCoInit) {
            HRESULT hr = ::CoInitializeEx(NULL, dwCoInit);
            if (FAILED(hr)) {
                ATLTRACE(TEXT("CoInitializeEx() failed in CComAutoInit constructor (hr=0x%08X).\n"), hr);
                AtlThrow(hr);
            }
        }

        // Uninitializes COM using CoUninitialize.
        ~CComAutoInit() { ::CoUninitialize(); }

        //
        // Ban copy
        //
    private:
        CComAutoInit(const CComAutoInit&);
    };

    // COM initialization and cleanup (must precede other COM related data members)
    CComAutoInit m_comInit;

    // Text to speech engine
    CComPtr<ISpVoice> m_tts;

    // Default voice token
    CComPtr<ISpObjectToken> m_voiceToken;

    QByteArray _lastSoundByteArray;
    AudioInjectorPointer _lastSoundAudioInjector;
};

#endif // hifi_SpeechScriptingInterface_h
