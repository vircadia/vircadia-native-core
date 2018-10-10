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
        ATLTRACE(TEXT("Text-to-speech creation failed.\n"));
        AtlThrow(hr);
    }

    //
    // Get token corresponding to default voice
    //
    hr = SpGetDefaultTokenFromCategoryId(SPCAT_VOICES, &m_voiceToken, FALSE);
    if (FAILED(hr)) {
        ATLTRACE(TEXT("Can't get default voice token.\n"));
        AtlThrow(hr);
    }

    //
    // Set default voice
    //
    hr = m_tts->SetVoice(m_voiceToken);
    if (FAILED(hr)) {
        ATLTRACE(TEXT("Can't set default voice.\n"));
        AtlThrow(hr);
    }

    WAVEFORMATEX fmt;
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nSamplesPerSec = 48000;
    fmt.wBitsPerSample = 16;
    fmt.nChannels = 1;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    BYTE* pcontent = new BYTE[1024 * 1000];

    cpIStream = SHCreateMemStream(NULL, 0);
    hr = outputStream->SetBaseStream(cpIStream, SPDFID_WaveFormatEx, &fmt);

    hr = m_tts->SetOutput(outputStream, true);
    if (FAILED(hr)) {
        ATLTRACE(TEXT("Can't set output stream.\n"));
        AtlThrow(hr);
    }
}

SpeechScriptingInterface::~SpeechScriptingInterface() {

}

void SpeechScriptingInterface::speakText(const QString& textToSpeak) {
    ULONG streamNumber;
    HRESULT hr = m_tts->Speak(reinterpret_cast<LPCWSTR>(textToSpeak.utf16()),
        SPF_IS_NOT_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK,
        &streamNumber);
    if (FAILED(hr)) {
        ATLTRACE(TEXT("Speak failed.\n"));
        AtlThrow(hr);
    }

    m_tts->WaitUntilDone(-1);

    outputStream->GetBaseStream(&cpIStream);
    ULARGE_INTEGER StreamSize;
    StreamSize.LowPart = 0;
    hr = IStream_Size(cpIStream, &StreamSize);

    DWORD dwSize = StreamSize.QuadPart;
    char* buf1 = new char[dwSize + 1];
    hr = IStream_Read(cpIStream, buf1, dwSize);

    QByteArray byteArray = QByteArray::QByteArray(buf1, (int)dwSize);
    AudioInjectorOptions options;

    options.position = DependencyManager::get<AvatarManager>()->getMyAvatarPosition();

    AudioInjector::playSound(byteArray, options);
}
