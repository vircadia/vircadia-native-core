//
//  Audio.cpp
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 4 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Audio.h"

#include <cassert>
#include <algorithm>
#include <iterator>

#include <PCMCodec.h>
#include <OpusCodec.h>

#include "audio/AudioClient.h"

namespace vircadia::client
{

    Audio::Audio() :
        codecs{
            std::make_shared<PCMCodec>(),
            std::make_shared<zLibCodec>(),
            std::make_shared<OpusCodec>()
        },
        allowedCodecs{codecs},
        selectedCodecName{""}
    {}

    void Audio::enable() {
        if (!isEnabled()) {
            DependencyManager::set<AudioClient>(allowedCodecs).data();
        }
    }

    const std::vector<std::shared_ptr<Codec>>& Audio::getCodecs() {
        return codecs;
    }

    const std::string& Audio::getSelectedCodecName() {
        selectedCodecName = DependencyManager::get<AudioClient>()->getSelectedCodecName();
        return selectedCodecName;
    }

    bool Audio::getIsMuted() const {
        return DependencyManager::get<AudioClient>()->getIsMuted();
    }

    bool Audio::getIsMutedByMixer() const {
        return DependencyManager::get<AudioClient>()->getIsMutedByMixer();
    }

    void Audio::setIsMuted(bool muted) {
        DependencyManager::get<AudioClient>()->setIsMuted(muted);
    }

    void Audio::setVantage(const vircadia_vantage_& vantage) {
        DependencyManager::get<AudioClient>()->setVantage(vantage);
    }

    void Audio::setBounds(const vircadia_bounds_& bounds) {
        DependencyManager::get<AudioClient>()->setBounds(bounds);
    }

    void Audio::setInputEcho(bool enabled) {
        DependencyManager::get<AudioClient>()->setInputEcho(enabled);
    }

    bool Audio::isEnabled() const {
        return DependencyManager::isSet<AudioClient>();
    }

    bool Audio::setCodecAllowed(std::string name, bool allow) {
        auto matchName = [&name] (const auto& codecPtr)
            { return codecPtr->getName().toStdString() == name; };

        auto codec = std::find_if(codecs.begin(), codecs.end(), matchName);
        if (codec == codecs.end()) {
            return false;
        }

        auto allowedCodec = std::find_if(allowedCodecs.begin(), allowedCodecs.end(), matchName);
        auto isAllowed = allowedCodec != allowedCodecs.end();

        if (allow != isAllowed) {
            if (allow) {
                allowedCodecs.push_back(*codec);
            } else if (isAllowed) {
                allowedCodecs.erase(allowedCodec);
            }

            DependencyManager::get<AudioClient>()->setSupportedCodecs(allowedCodecs);
        }

        return true;
    }

    void Audio::setInput(const AudioFormat& format) {
        auto client =
        DependencyManager::get<AudioClient>();
        client->setInputFormat(format);
    }

    void Audio::setOutput(const AudioFormat& format) {
        DependencyManager::get<AudioClient>()->setOutputFormat(format);
    }

    AudioClient* Audio::getInputContext() {
        return DependencyManager::get<AudioClient>()->getInput();
    }

    AudioClient* Audio::getOutputContext() {
        return DependencyManager::get<AudioClient>()->getOutput();
    }

    void Audio::destroy() {
        if (isEnabled()) {
            DependencyManager::destroy<AudioClient>();
        }
    }

} // namespace vircadia::client
