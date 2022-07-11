//
//  AudioClient.h
//  libraries/vircadia-client/src/internal/audio
//
//  Created by Nshan G. on 4 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AUDIO_AUDIOCLIENT_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AUDIO_AUDIOCLIENT_H

#include <QObject>

#include <AudioPacketHandler.h>

#include "../../audio.h"

namespace vircadia::client
{

    /// @private
    class AudioClient : public QObject, public Dependency, public AudioPacketHandler<AudioClient> {
        Q_OBJECT
    public:

        AudioClient(const std::vector<std::shared_ptr<Codec>>& supportedCodecs);

        void setSupportedCodecs(const std::vector<std::shared_ptr<Codec>>&);
        const std::vector<std::shared_ptr<Codec>>& getSupportedCodecs() const;

        void setInputFormat(QAudioFormat);
        void setOutputFormat(QAudioFormat);

        AudioClient* getInput();
        AudioClient* getOutput();

        std::string getSelectedCodecName() const;

        bool getIsMuted() const;
        bool getIsMutedByMixer() const;

        void setVantage(vircadia_vantage_);
        void setBounds(vircadia_bounds_);
        void setInputEcho(bool enabled);
        void setIsMuted(bool muted);

        AudioOutputIODevice& getOutputIODevice();

    private slots:
        void update();

    private:

        void onMutedByMixer();

        friend class AudioPacketHandler<AudioClient>;

        glm::vec3 position;
        glm::quat orientation;
        std::vector<std::shared_ptr<Codec>> codecs;

        QTimer updateTimer;

        std::vector<std::shared_ptr<Codec>> codecsIn;
        QAudioFormat inputFormat;
        QAudioFormat outputFormat;
        AudioClient* input;
        AudioClient* output;
        std::string selectedCodecName;
        bool isMuted;
        bool isMutedByMixer;
        bool inputEcho;
        vircadia_vantage_ vantage;
        vircadia_bounds_ bounds;


        mutable std::mutex inout;
    };

} // namespace vircadia::client

#endif /* end of include guard */
