//
//  Context.h
//  libraries/client/src/internal
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_CONTEXT_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_CONTEXT_H

#include <vector>
#include <string>
#include <array>
#include <thread>
#include <atomic>
#include <future>

#include <NodeList.h>

#include "../client.h"

class QCoreApplication;
class QString;

namespace vircadia::client {

    /// @private
    struct NodeData {
        uint8_t type;
        bool active;
        std::string address;
        std::array<uint8_t, 16> uuid;
    };

    /// @private
    class Context {

    public:
        Context(NodeList::Params nodeListParams, const char* userAgent, vircadia_client_info info);

        bool ready();

        ~Context();

        const std::vector<NodeData> getNodes();
        void updateNodes();

        bool isConnected();

        void connect(const QString& address);

    private:
        std::thread appThraed {};
        std::atomic<QCoreApplication*> app {};
        std::vector<NodeData> nodes {};
        std::promise<void> qtInitialized {};

        int argc;
        char argvData[18];
        char* argv;
    };

    extern std::list<Context> contexts;

    int vircadiaContextValid(int id);
    int vircadiaContextReady(int id);

} // namespace vircadia::client

#endif /* end of include guard */
