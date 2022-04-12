//
//  node_list.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 15 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "node_list.h"

#include "internal/Error.h"
#include "internal/Context.h"

using namespace vircadia::client;

VIRCADIA_CLIENT_DYN_API
int vircadia_connect(int id, const char* address) {
    return chain(checkContextReady(id), [&](auto) {
        std::next(std::begin(contexts), id)->connect(address);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_connection_status(int id) {
    return chain(checkContextReady(id), [&](auto) {
        return std::next(std::begin(contexts), id)->isConnected()
            ? 1
            : 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_update_nodes(int id) {
    return chain(checkContextReady(id), [&](auto) {
        std::next(std::begin(contexts), id)->updateNodes();
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_node_count(int id) {
    return chain(checkContextReady(id), [&](auto) -> int {
        return std::next(std::begin(contexts), id)->getNodes().size();
    });
}

VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_node_uuid(int id, int index) {
    return chain(checkContextReady(id), [&](auto) {
        const auto& nodes = std::next(std::begin(contexts), id)->getNodes();
        return chain(checkIndexValid(nodes, index, ErrorCode::NODE_INVALID), [&](auto) {
            return std::next(std::begin(nodes), index)->uuid.data();
        });
    });

}
