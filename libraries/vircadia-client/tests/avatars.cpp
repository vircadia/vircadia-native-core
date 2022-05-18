//
//  messages.cpp
//  libraries/vircadia-client/tests
//
//  Created by Nshan G. on 30 March 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
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

    const auto avatarURLs = std::array{
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Kim/fbx/Kim.fst"s,
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Mason/fbx/Mason.fst"s,
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Mike/fbx/Mike.fst"s,
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Sean/fbx/Sean.fst"s,
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Summer/fbx/Summer.fst"s,
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Tanya/fbx/Tanya.fst"s
    };

    unsigned randomIndex = std::chrono::steady_clock::now().time_since_epoch().count() % avatarURLs.size();

    const int context = vircadia_create_context(vircadia_context_defaults());
    vircadia_connect(context, "localhost");

    vircadia_vector global_position {0,0,0};

    {
        auto cleanup = defer([context](){
            REQUIRE(vircadia_destroy_context(context) == 0);
        });

        REQUIRE(vircadia_set_my_avatar_display_name(context, "Client Unit Test Avatar") == vircadia_error_avatars_disabled());
        REQUIRE(vircadia_update_avatars(context) == vircadia_error_avatars_disabled());

        REQUIRE(vircadia_enable_avatars(context) == 0);
        REQUIRE(vircadia_set_my_avatar_display_name(context, "Client Unit Test Avatar") == 0);
        REQUIRE(vircadia_set_my_avatar_skeleton_model_url(context, avatarURLs[randomIndex].c_str()) == 0);

        for (int i = 0; i < 100; ++i) {

            REQUIRE(vircadia_update_avatars(context) == 0);
            REQUIRE(vircadia_set_my_avatar_global_position(context, global_position) == 0);
            global_position.x += 0.1f;
            global_position.y += 0.1f;
            global_position.z += 0.1f;

            std::this_thread::sleep_for(100ms);
        }
    }

}
