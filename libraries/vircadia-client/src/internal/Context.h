//
//  Context.h
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_CONTEXT_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_CONTEXT_H

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <future>
#include <mutex>

#include <NodeList.h>

#include "../context.h"
#include "Common.h"
#include "Messages.h"
#include "Audio.h"
#include "Avatars.h"

class QCoreApplication;

namespace vircadia::client {

    /// @private
    struct NodeData {
        uint8_t type;
        bool active;
        std::string address;
        UUID uuid;

        float inboundPPS;
        float outboundPPS;
        float inboundKbps;
        float outboundKbps;
    };

    /// @private
    class Context {

    public:
        Context(NodeList::Params nodeListParams, const char* userAgent, vircadia_client_info info);

        bool ready() const;

        ~Context();

        const std::vector<NodeData>& getNodes() const;
        void updateNodes();

        bool isConnected() const;

        void connect(const QString& address) const;

        Messages& messages();
        const Messages& messages() const;

        Audio& audio();
        const Audio& audio() const;

        Avatars& avatars();
        const Avatars& avatars() const;

        const UUID& getSessionUUID() const;

    private:
        std::thread appThread {};
        std::atomic<QCoreApplication*> app {};
        std::vector<NodeData> nodes {};
        UUID sessionUUID {};
        std::promise<void> qtInitialized {};

        int argc;
        char argvData[18];
        char* argv;

        Messages messages_;
        Audio audio_;
        Avatars avatars_;
    };

    extern std::list<Context> contexts;

    int checkContextValid(int id);
    int checkContextReady(int id);

} // namespace vircadia::client

#endif /* end of include guard */
