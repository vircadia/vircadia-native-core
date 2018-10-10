//
//  SpeechScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Zach Fox on 2018-10-10.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SpeechScriptingInterface.h"
#include "avatar/AvatarManager.h"
#include <AudioInjector.h>

SpeechScriptingInterface::SpeechScriptingInterface() {
    //
    // Create text to speech engine
    //
    HRESULT hr = m_tts.CoCreateInstance(CLSID_SpVoice);
    if (FAILED(hr)) {
        qDebug() << "Text-to-speech engine creation failed.";
    }

    //
    // Get token corresponding to default voice
    //
    hr = SpGetDefaultTokenFromCategoryId(SPCAT_VOICES, &m_voiceToken, FALSE);
    if (FAILED(hr)) {
        qDebug() << "Can't get default voice token.";
    }

    //
    // Set default voice
    //
    hr = m_tts->SetVoice(m_voiceToken);
    if (FAILED(hr)) {
        qDebug() << "Can't set default voice.";
    }
}

SpeechScriptingInterface::~SpeechScriptingInterface() {

}

class ReleaseOnExit {
public:
    ReleaseOnExit(IUnknown* p) : m_p(p) {}
    ~ReleaseOnExit() {
        if (m_p) {
            m_p->Release();
        }
    }

private:
    IUnknown* m_p;
};

void SpeechScriptingInterface::speakText(const QString& textToSpeak) {
    WAVEFORMATEX fmt;
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 16;
    fmt.nChannels = 1;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    IStream* pStream = NULL;

    ISpStream* pSpStream = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SpStream, nullptr, CLSCTX_ALL, __uuidof(ISpStream), (void**)&pSpStream);
    if (FAILED(hr)) {
        qDebug() << "CoCreateInstance failed.";
    }
    ReleaseOnExit rSpStream(pSpStream);

    pStream = SHCreateMemStream(NULL, 0);
    if (nullptr == pStream) {
        qDebug() << "SHCreateMemStream failed.";
    }

    hr = pSpStream->SetBaseStream(pStream, SPDFID_WaveFormatEx, &fmt);
    if (FAILED(hr)) {
        qDebug() << "Can't set base stream.";
    }

    hr = m_tts->SetOutput(pSpStream, true);
    if (FAILED(hr)) {
        qDebug() << "Can't set output stream.";
    }

    ReleaseOnExit rStream(pStream);

    ULONG streamNumber;
    hr = m_tts->Speak(reinterpret_cast<LPCWSTR>(textToSpeak.utf16()),
        SPF_IS_NOT_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK,
        &streamNumber);
    if (FAILED(hr)) {
        qDebug() << "Speak failed.";
    }

    m_tts->WaitUntilDone(-1);

    hr = pSpStream->GetBaseStream(&pStream);
    if (FAILED(hr)) {
        qDebug() << "Couldn't get base stream.";
    }

    hr = IStream_Reset(pStream);
    if (FAILED(hr)) {
        qDebug() << "Couldn't reset stream.";
    }

    ULARGE_INTEGER StreamSize;
    StreamSize.LowPart = 0;
    hr = IStream_Size(pStream, &StreamSize);

    DWORD dwSize = StreamSize.QuadPart;
    char* buf1 = new char[dwSize + 1];
    memset(buf1, 0, dwSize + 1);

    hr = IStream_Read(pStream, buf1, dwSize);
    if (FAILED(hr)) {
        qDebug() << "Couldn't read from stream.";
    }

    QByteArray byteArray = QByteArray::QByteArray(buf1, dwSize);

    AudioInjectorOptions options;
    options.position = DependencyManager::get<AvatarManager>()->getMyAvatarPosition();

    AudioInjector::playSound(byteArray, options);
}
