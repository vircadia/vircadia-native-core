//
//  client.cpp
//  libraries/client/src
//
//  Created by Nshan G. on 15 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "client.h"

#include <list>

#include <nlohmann/json.hpp>

#include "version.h"
#include "internal/Context.h"

namespace vircadia { namespace client
{
    std::list<Context> contexts;
}} // namespace vircadia::client


using namespace vircadia::client;

VIRCADIA_CLIENT_DYN_API
vircadia_context_params vircadia_context_defaults() {
    return {
        -1, -1,
        nullptr,
        nullptr,
        {
            "VircadiaClient",
            "Vircadia",
            "vircadia.com",
            vircadia_client_version()->full
        }
    };
}

VIRCADIA_CLIENT_DYN_API
int vircadia_create_context(vircadia_context_params params) {
    if (!contexts.empty()) {
        return -1;
    }

    nlohmann::json platform_info{};
    if (params.platform_info != nullptr) {
        auto parsed = nlohmann::json::parse(params.platform_info, nullptr, false);
        if (!parsed.is_discarded()) {
            platform_info = parsed;
        }
    }

    int index = contexts.size();
    vircadia::client::contexts.emplace_back(
        NodeList::Params{
            NodeType::Agent,
            {params.listenPort, params.dtlsListenPort},
            platform_info
        },
        params.user_agent,
        params.client_info);
    return index;
}

int vircadia_context_ready(int id) {
    auto begin = std::begin(contexts);
    if ( id < 0 || id >= static_cast<int>(contexts.size()) ) {
        return -1;
    } else {
        return std::next(begin, id)->ready() ? 0 : -2;
    }
}

VIRCADIA_CLIENT_DYN_API
int vircadia_destroy_context(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    contexts.erase(context);
    return 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_connect(int id, const char* address) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    context->connect(address);
    return 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_connection_status(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);

    return context->isConnected() ? 1 : 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_update_nodes(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    context->updateNodes();
    return 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_node_count(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    return context->getNodes().size();
}

VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_node_uuid(int id, int index) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return nullptr;
    }

    auto context = std::next(std::begin(contexts), id);
    const auto& nodes = context->getNodes();

    if ( index < 0 || id >= static_cast<int>(nodes.size()) ) {
        return nullptr;
    }

    auto node = std::next(std::begin(nodes), index);
    return node->uuid.data();
}
