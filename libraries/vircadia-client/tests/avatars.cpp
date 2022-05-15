//
//  messages.cpp
//  libraries/vircadia-client/tests
//
//  Created by Nshan G. on 30 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "../src/context.h"
#include "../src/node_list.h"
#include "../src/node_types.h"
#include "../src/avatars.h"
#include "../src/error.h"

#include <thread>
#include <chrono>
#include <string>
#include <cstring>

#include <catch2/catch.hpp>

#include "common.h"

using namespace std::literals;

TEST_CASE("Client API avatar functionality.", "[client-api-avatars]") {

    const int context = vircadia_create_context(vircadia_context_defaults());
    vircadia_connect(context, "localhost");

    REQUIRE(vircadia_enable_avatars(context) == 0);
    REQUIRE(vircadia_set_my_avatar_display_name(context, "Client Unit Test Avatar") == 0);

    {
        auto cleanup = defer([context](){
            REQUIRE(vircadia_destroy_context(context) == 0);
        });

        bool avatar_mixer_active = false;
        for (int i = 0; i < 100; ++i) {
            int status = vircadia_connection_status(context);
            if (status == 1) {

                REQUIRE(vircadia_update_nodes(context) == 0);
                const int node_count = vircadia_node_count(context);
                for (int i = 0; i < node_count; ++i) {
                    if (vircadia_node_type(context, i) == vircadia_avatar_mixer_node()) {
                        avatar_mixer_active = vircadia_node_active(context, i);
                    }
                }


                if (avatar_mixer_active) {
                    REQUIRE(vircadia_update_avatars(context) == 0);
                }

            }

            std::this_thread::sleep_for(100ms);
        }

    }

}
