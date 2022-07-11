//
//  Audio.h
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 4 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_H

#include <vector>
#include <memory>
#include <mutex>

class Codec;
class QAudioFormat;
struct vircadia_vantage_;
struct vircadia_bounds_;

namespace vircadia::client {

    class AudioClient;

    /// @private
    class Audio {
    public:
        Audio();
        void enable();
        bool isEnabled() const;
        const std::vector<std::shared_ptr<Codec>>& getCodecs();
        bool setCodecAllowed(std::string name, bool allow);

        const std::string& getSelectedCodecName();
        bool getIsMuted() const;
        bool getIsMutedByMixer() const;
        void setIsMuted(bool);
        void setVantage(const vircadia_vantage_&);
        void setBounds(const vircadia_bounds_&);
        void setInputEcho(bool enabled);

        void setInput(const QAudioFormat&);
        void setOutput(const QAudioFormat&);

        AudioClient* getInputContext();
        AudioClient* getOutputContext();

        void destroy();
    private:

        std::vector<std::shared_ptr<Codec>> codecs;
        std::vector<std::shared_ptr<Codec>> allowedCodecs;

        std::string selectedCodecName;
    };

} // namespace vircadia::client

#endif /* end of include guard */
