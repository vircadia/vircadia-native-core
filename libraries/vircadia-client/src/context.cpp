//
//  context.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 15 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "context.h"

#include <nlohmann/json.hpp>

#include "version.h"
#include "internal/Error.h"
#include "internal/Context.h"

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
        return toInt(ErrorCode::CONTEXT_EXISTS);
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

VIRCADIA_CLIENT_DYN_API
int vircadia_destroy_context(int id) {
    return chain(checkContextValid(id), [&](auto) {
        contexts.erase(std::next(std::begin(contexts), id));
        return 0;
    });
}
