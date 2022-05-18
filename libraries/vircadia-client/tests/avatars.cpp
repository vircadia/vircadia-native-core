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
#include <cmath>

#include <catch2/catch.hpp>

#include "common.h"

using namespace std::literals;

const float pi = std::acos(-1);

vircadia_quaternion axis_angle(vircadia_vector axis, float angle) {
    const float s = std::sin(angle/2);
    return {
        s * axis.x,
        s * axis.y,
        s * axis.z,
        std::cos(angle/2)
    };
}

TEST_CASE("Client API avatar functionality.", "[client-api-avatars]") {

    const auto avatar_urls = array{
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Kim/fbx/Kim.fst",
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Mason/fbx/Mason.fst",
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Mike/fbx/Mike.fst",
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Sean/fbx/Sean.fst",
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Summer/fbx/Summer.fst",
        "https://cdn-1.vircadia.com/us-e-1/Bazaar/Avatars/Tanya/fbx/Tanya.fst"
    };

    unsigned random = std::chrono::steady_clock::now().time_since_epoch().count() % avatar_urls.size();

    const int context = vircadia_create_context(vircadia_context_defaults());
    vircadia_connect(context, "localhost");

    vircadia_vector global_position {0,0,0};
    vircadia_bounds bounding_box {{1,1,1}, {0,0,0}};
    float rotation = 0;
    float scale = 1;

    {
        auto cleanup = defer([context](){
            REQUIRE(vircadia_destroy_context(context) == 0);
        });

        REQUIRE(vircadia_set_my_avatar_display_name(context, "Client Unit Test Avatar") == vircadia_error_avatars_disabled());
        REQUIRE(vircadia_update_avatars(context) == vircadia_error_avatars_disabled());

        REQUIRE(vircadia_enable_avatars(context) == 0);
        REQUIRE(vircadia_set_my_avatar_display_name(context, "Client Unit Test Avatar") == 0);
        REQUIRE(vircadia_set_my_avatar_skeleton_model_url(context, avatar_urls[random]) == 0);
        REQUIRE(vircadia_set_my_avatar_bounding_box(context, bounding_box) == 0);

        for (int i = 0; i < 1000; ++i) {

            REQUIRE(vircadia_update_avatars(context) == 0);
            REQUIRE(vircadia_set_my_avatar_global_position(context, global_position) == 0);
            global_position.x += 0.01f;
            global_position.y += 0.01f;
            global_position.z += 0.01f;

            REQUIRE(vircadia_set_my_avatar_orientation(context, axis_angle({0,1,0},rotation)) == 0);
            rotation += pi/250;

            REQUIRE(vircadia_set_my_avatar_scale(context, scale) == 0);
            scale += 0.001f;

            std::this_thread::sleep_for(10ms);
        }
    }

}
