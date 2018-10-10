//  SpeechScriptingInterface.h
//  interface/src/scripting
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

#include <sapi.h>      // SAPI
#include <sphelper.h>  // SAPI Helper

class SpeechScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    SpeechScriptingInterface();
    ~SpeechScriptingInterface();

    Q_INVOKABLE void speakText(const QString& textToSpeak);

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

    CComPtr<ISpStream> outputStream;
    CComPtr<IStream> cpIStream;
};

#endif // hifi_SpeechScriptingInterface_h
