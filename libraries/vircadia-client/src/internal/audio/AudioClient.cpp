//
//  AudioClient.cpp
//  libraries/vircadia-client/src/internal/audio
//
//  Created by Nshan G. on 4 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioClient.h"

#include <AudioPacketHandler.hpp>

namespace vircadia::client
{
    // FIXME: avatar API PR also has these, need to be moved to a
    // common header once merged
    glm::vec3 glmVec3From_(vircadia_vector_ v) {
        return {v.x, v.y, v.z};
    }

    glm::quat glmQuatFrom_(vircadia_quaternion_ q) {
        return {q.w, q.x, q.y, q.z};
    }

    AudioClient::AudioClient(const std::vector<std::shared_ptr<Codec>>& supportedCodecs) :
        AudioPacketHandler<AudioClient>(),
        position(),
        orientation(),
        codecs(supportedCodecs),
        updateTimer(this),
        codecsIn(codecs),
        inputFormat(),
        outputFormat(),
        input(nullptr),
        output(nullptr),
        selectedCodecName(),
        isMuted(false),
        isMutedByMixer(false),
        inputEcho(false),
        vantage{{0.f, 0.f, 0.f}, {0.f,0.f,0.f,1.f}},
        bounds{{0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}}
    {

        setPositionGetter([this]() { return position; });
        setOrientationGetter([this]() { return orientation; });

        setCustomDeleter([](Dependency* dependency){
            static_cast<AudioClient*>(dependency)->deleteLater();
        });

        startThread();
    }

    void AudioClient::onStart() {
        connect(&updateTimer, &QTimer::timeout, this, &AudioClient::update);
        updateTimer.start();
    }

    void AudioClient::setInputFormat(AudioFormat format) {
        std::scoped_lock lock(inout);
        inputFormat = format;
        input = nullptr;
    }

    void AudioClient::setOutputFormat(AudioFormat format) {
        std::scoped_lock lock(inout);
        outputFormat = format;
        output = nullptr;
    }

    AudioClient* AudioClient::getInput() {
        std::scoped_lock lock(inout);
        return input;
    }

    AudioClient* AudioClient::getOutput() {
        std::scoped_lock lock(inout);
        return output;
    }

    void AudioClient::update() {
        {
            std::scoped_lock lock(inout);

            if (inputFormat != _inputFormat) {
                cleanupInput();
                if (inputFormat.isValid()) {
                    setupInput(inputFormat);
                } else {
                    setupDummyInput();
                }
            }

            if (_inputFormat.isValid()) {
                input = this;
            }

            if (outputFormat != _outputFormat) {
                cleanupOutput();
                if (outputFormat.isValid()) {
                    setupOutput(outputFormat);
                }
            }

            if (_outputFormat.isValid()) {
                output = this;
            }

            selectedCodecName = _selectedCodecName.toStdString();

            if (codecs != codecsIn) {
                codecs = codecsIn;
                negotiateAudioFormat();
            }

            position = glmVec3From_(vantage.position);
            orientation = glmQuatFrom_(vantage.rotation);
            avatarBoundingBoxCorner = glmVec3From_(bounds.offset);
            avatarBoundingBoxScale = glmVec3From_(bounds.dimensions);

            _isMuted = isMuted || isMutedByMixer;
            setServerEcho(inputEcho);
        }

        if (_inputFormat.isValid()) {
            sendInput();
        }
    }

    void AudioClient::onMutedByMixer() {
        std::scoped_lock lock(inout);
        isMutedByMixer = true;
    }

    void AudioClient::setSupportedCodecs(const std::vector<std::shared_ptr<Codec>>& supportedCodecs) {
        std::scoped_lock lock(inout);
        codecsIn = supportedCodecs;
    }

    const std::vector<std::shared_ptr<Codec>>& AudioClient::getSupportedCodecs() const {
        return codecs;
    }

    AudioClient::AudioOutputIODevice& AudioClient::getOutputIODevice() {
        return _audioOutputIODevice;
    }

    std::string AudioClient::getSelectedCodecName() const {
        std::scoped_lock lock(inout);
        return _selectedCodecName.toStdString();
    }

    bool AudioClient::getIsMuted() const {
        std::scoped_lock lock(inout);
        return isMuted;
    }

    bool AudioClient::getIsMutedByMixer() const {
        std::scoped_lock lock(inout);
        return isMutedByMixer;
    }

    void AudioClient::setVantage(vircadia_vantage_ value) {
        std::scoped_lock lock(inout);
        vantage = value;
    }

    void AudioClient::setBounds(vircadia_bounds_ value) {
        std::scoped_lock lock(inout);
        bounds = value;
    }

    void AudioClient::setInputEcho(bool enabled) {
        std::scoped_lock lock(inout);
        inputEcho = enabled;
    }

    void AudioClient::setIsMuted(bool muted) {
        std::scoped_lock lock(inout);
        isMuted = muted;
    }

} // namespace vircadia::client

template class AudioPacketHandler<vircadia::client::AudioClient>;
