//
//  TTSScriptingInterface.cpp
//  libraries/audio-client/src/scripting
//
//  Created by Zach Fox on 2018-10-10.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TTSScriptingInterface.h"
#include "avatar/AvatarManager.h"

TTSScriptingInterface::TTSScriptingInterface() {
#ifdef WIN32
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

    _lastSoundAudioInjectorUpdateTimer.setSingleShot(true);
    connect(&_lastSoundAudioInjectorUpdateTimer, &QTimer::timeout, this, &TTSScriptingInterface::updateLastSoundAudioInjector);
#endif
}

TTSScriptingInterface::~TTSScriptingInterface() {
}

#ifdef WIN32
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
#endif

const int INJECTOR_INTERVAL_MS = 100;
void TTSScriptingInterface::updateLastSoundAudioInjector() {
    if (_lastSoundAudioInjector) {
        AudioInjectorOptions options;
        options.position = DependencyManager::get<AvatarManager>()->getMyAvatarPosition();
        _lastSoundAudioInjector->setOptions(options);
        _lastSoundAudioInjectorUpdateTimer.start(INJECTOR_INTERVAL_MS);
    }
}

void TTSScriptingInterface::speakText(const QString& textToSpeak) {
#ifdef WIN32
    WAVEFORMATEX fmt;
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nSamplesPerSec = AudioConstants::SAMPLE_RATE;
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
    hr = m_tts->Speak(reinterpret_cast<LPCWSTR>(textToSpeak.utf16()), SPF_IS_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK,
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
    _lastSoundByteArray.resize(dwSize);

    hr = IStream_Read(pStream, _lastSoundByteArray.data(), dwSize);
    if (FAILED(hr)) {
        qDebug() << "Couldn't read from stream.";
    }

    AudioInjectorOptions options;
    options.position = DependencyManager::get<AvatarManager>()->getMyAvatarPosition();

    if (_lastSoundAudioInjector) {
        _lastSoundAudioInjector->stop();
        _lastSoundAudioInjectorUpdateTimer.stop();
    }

    uint32_t numChannels = 1;
    uint32_t numSamples = (uint32_t)_lastSoundByteArray.size() / sizeof(AudioData::AudioSample);
    auto samples = reinterpret_cast<AudioData::AudioSample*>(_lastSoundByteArray.data());
    auto newAudioData = AudioData::make(numSamples, numChannels, samples);
    _lastSoundAudioInjector = AudioInjector::playSoundAndDelete(newAudioData, options);

    _lastSoundAudioInjectorUpdateTimer.start(INJECTOR_INTERVAL_MS);
#else
    qDebug() << "Text-to-Speech isn't currently supported on non-Windows platforms.";
#endif
}

void TTSScriptingInterface::stopLastSpeech() {
    if (_lastSoundAudioInjector) {
        _lastSoundAudioInjector->stop();
        _lastSoundAudioInjector = NULL;
    }
}
