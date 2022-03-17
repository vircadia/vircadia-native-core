//
//  client.cpp
//  libraries/client/tests
//
//  Created by Nshan G. on 17 Mar 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <../src/client.h>

#include <thread>
#include <chrono>
#include <catch2/catch.hpp>

using namespace std::literals;

TEST_CASE("Client API context creation and domain server connection APIs.", "[client-api-context]") {

    const int context = vircadia_create_context(vircadia_context_defaults());
    REQUIRE(context >= 0);

    {
        int attempts = 0;
        while(vircadia_context_ready(context) < 0) {
            REQUIRE(attempts++ < 10);
            std::this_thread::sleep_for(10ms);
        }
    }


    vircadia_connect(context, "localhost");

    {
        for (int i = 0; i < 10; ++i) {
            int status = vircadia_connection_status(context);
            REQUIRE((status == 1 || status == 0));
            if (status == 1) {
                for (int i = 0; i < 10; ++i) {
                    REQUIRE(vircadia_update_nodes(context) == 0);
                    REQUIRE(vircadia_node_count(context) >= 0);
                    for (int i = 0; i < vircadia_node_count(context) - 1; ++i) {
                        for (int j = i + 1; j < vircadia_node_count(context); ++j) {
                            auto one = vircadia_node_uuid(context, i);
                            auto other = vircadia_node_uuid(context, j);
                            REQUIRE_FALSE(std::equal(one, one + 16, other));
                        }
                    }
                    if (vircadia_node_count(context) > 3) {
                        break;
                    }
                    std::this_thread::sleep_for(1s);
                }
                break;
            }

            std::this_thread::sleep_for(1s);

        }
    }

    const int secondContext = vircadia_create_context(vircadia_context_defaults());
    REQUIRE(secondContext == -1);

    const int secondDestroy = vircadia_destroy_context(secondContext);
    REQUIRE(secondDestroy == -1);

    const int badDestroy = vircadia_destroy_context(context + 1);
    REQUIRE(badDestroy == -1);

    const int destroy = vircadia_destroy_context(context);
    REQUIRE(destroy == 0);

    const int doubleDestroy = vircadia_destroy_context(context);
    REQUIRE(doubleDestroy == -1);

}
