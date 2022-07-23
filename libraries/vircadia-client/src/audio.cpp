//
//  audio.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 9 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "audio.h"


#include "internal/audio/AudioClient.h"
#include "internal/Error.h"
#include "internal/Context.h"

using namespace vircadia::client;


VIRCADIA_CLIENT_DYN_API
int vircadia_enable_audio(int context_id) {
    return chain(checkContextReady(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->audio().enable();
        return 0;
    });
}

int checkAudioEnabled(int id) {
    return chain(checkContextReady(id), [&](auto) {
        return std::next(std::begin(contexts), id)->audio().isEnabled()
            ? 0
            : toInt(ErrorCode::AUDIO_DISABLED);
    });
}

int validateAudioFormat(const vircadia_audio_format& format, bool input = false) {
    if (
        !(format.sample_type == AudioFormat::Signed16 || format.sample_type == AudioFormat::Float) ||
        format.sample_rate <= 0 ||
        format.channel_count <= 0 ||
        (input && format.channel_count > 2)
    ) {
        return toInt(ErrorCode::AUDIO_FORMAT_INVALID);
    } else {
        return 0;
    }
}

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_selected_audio_codec_name(int context_id) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        return std::next(std::begin(contexts), context_id)->audio().getSelectedCodecName().c_str();
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_codec_params(int context_id, const char* codec, vircadia_audio_codec_params params) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        auto& audio = std::next(std::begin(contexts), context_id)->audio();
        if (audio.setCodecAllowed(codec, params.allowed)) {
            return 0;
        } else {
            return toInt(ErrorCode::AUDIO_CODEC_INVALID);
        }
    });
}

AudioFormat audioFormatFrom(const vircadia_audio_format& format) {
    AudioFormat result{};
    result.sampleType = AudioFormat::SampleTag(format.sample_type);
    result.sampleRate = format.sample_rate;
    result.channelCount = format.channel_count;
    return result;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_format(int context_id, vircadia_audio_format format) {
    return chain({
        checkAudioEnabled(context_id),
        validateAudioFormat(format, true)
    }, [&](auto) {
        std::next(std::begin(contexts), context_id)->audio().setInput(audioFormatFrom(format));
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_output_format(int context_id, vircadia_audio_format format) {
    return chain({
        checkAudioEnabled(context_id),
        validateAudioFormat(format)
    }, [&](auto) {
        std::next(std::begin(contexts), context_id)->audio().setOutput(audioFormatFrom(format));
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
uint8_t* vircadia_get_audio_input_context(int context_id) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        auto& audio = std::next(std::begin(contexts), context_id)->audio();
        return reinterpret_cast<uint8_t*>(audio.getInputContext());
    });
}

VIRCADIA_CLIENT_DYN_API
uint8_t* vircadia_get_audio_output_context(int context_id) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        auto& audio = std::next(std::begin(contexts), context_id)->audio();
        return reinterpret_cast<uint8_t*>(audio.getOutputContext());
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_data(uint8_t* audio_context, const uint8_t* data, int size) {
    auto audioClient = reinterpret_cast<AudioClient*>(audio_context);
    if (data == nullptr || size < 0) {
        return toInt(ErrorCode::ARGUMENT_INVALID);
    } else if (audioClient == nullptr) {
        return toInt(ErrorCode::AUDIO_CONTEXT_INVALID);
    } else {
        audioClient->handleMicAudioInput(reinterpret_cast<const char*>(data), size);
        return 0;
    }
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_audio_output_data(uint8_t* audio_context, uint8_t* data, int size) {
    auto audioClient = reinterpret_cast<AudioClient*>(audio_context);
    if (data == nullptr || size < 0) {
        return toInt(ErrorCode::ARGUMENT_INVALID);
    } else if (audioClient == nullptr) {
        return toInt(ErrorCode::AUDIO_CONTEXT_INVALID);
    }

    int bytesWritten = audioClient->getOutputIODevice().read(reinterpret_cast<char*>(data), size);
    if (bytesWritten== -1) {
        return toInt(ErrorCode::AUDIO_CONTEXT_INVALID);
    }

    return bytesWritten;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_bounds(int context_id, vircadia_bounds bounds) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->audio().setBounds(bounds);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_vantage(int context_id, vircadia_vantage vantage) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->audio().setVantage(vantage);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_echo(int context_id, uint8_t enabled) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->audio().setInputEcho(enabled);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_muted(int context_id, uint8_t muted) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->audio().setIsMuted(muted);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_audio_input_muted_by_mixer(int context_id) {
    return chain(checkAudioEnabled(context_id), [&](auto) {
        return std::next(std::begin(contexts), context_id)->audio().getIsMutedByMixer() ? 1 : 0;
    });
}


