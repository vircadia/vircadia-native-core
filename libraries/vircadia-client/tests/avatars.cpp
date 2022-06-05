//
//  avatars.cpp
//  libraries/vircadia-client/tests
//
//  Created by Nshan G. on 16 May 2022.
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
#include <algorithm>

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

TEST_CASE("Client API avatar functionality - sender.", "[client-api-avatars-sender]") {

    // TODO: this will not work if multiple tests are ran against a single
    // server, should use unique id per test run instead
    const auto name_prefix = "Client Unit Test Avatar"s;
    const auto send_string = "(sending test data)"s;

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

    const uint8_t* session_uuid = nullptr;

    {
        auto cleanup = defer([context](){
            REQUIRE(vircadia_destroy_context(context) == 0);
        });

        REQUIRE(vircadia_set_my_avatar_display_name(context, name_prefix.c_str()) == vircadia_error_avatars_disabled());
        REQUIRE(vircadia_update_avatars(context) == vircadia_error_avatars_disabled());

        REQUIRE(vircadia_enable_avatars(context) == 0);
        REQUIRE(vircadia_set_my_avatar_display_name(context, name_prefix.c_str()) == 0);
        REQUIRE(vircadia_set_my_avatar_skeleton_model_url(context, avatar_urls[random]) == 0);
        REQUIRE(vircadia_set_my_avatar_bounding_box(context, bounding_box) == 0);
        REQUIRE(vircadia_set_my_avatar_look_at_snapping(context, 1) == 0);

        const uint8_t* copying = nullptr;
        const uint8_t* receiver = nullptr;

        for (int i = 0; i < 1000; ++i) {

            REQUIRE(vircadia_update_nodes(context) == 0);
            session_uuid = vircadia_client_get_session_uuid(context);
            REQUIRE(session_uuid != nullptr);

            REQUIRE(vircadia_update_avatars(context) == 0);

            auto avatar_count = vircadia_get_avatar_count(context);
            REQUIRE(avatar_count >= 0);

            bool receiver_active = false;

            for (int avatar = 0; avatar != avatar_count; ++avatar) {
                const uint8_t* uuid = vircadia_get_avatar_uuid(context, avatar);
                REQUIRE(uuid != nullptr);
                auto display_name = vircadia_get_avatar_display_name(context, avatar);
                REQUIRE(display_name != nullptr);

                bool is_me = std::equal(uuid, uuid + 16, session_uuid);
                if (is_me) {
                    continue;
                }

                bool starts_with_prefix = std::search(display_name, display_name + std::strlen(display_name),
                    name_prefix.begin(), name_prefix.end()) == display_name;

                if (copying != nullptr) {
                    // for manual testing, replicates another avatar
                    if (std::equal(copying, copying + 16, uuid)) {
                        auto joint_count = vircadia_get_avatar_joint_count(context, avatar);
                        REQUIRE(joint_count >= 0);
                        REQUIRE(vircadia_set_my_avatar_joint_count(context, joint_count) == 0);
                        for (int j = 0; j != joint_count; ++j) {
                            auto joint = vircadia_get_avatar_joint(context, avatar, j);
                            REQUIRE(joint != nullptr);
                            REQUIRE(vircadia_set_my_avatar_joint(context, j, *joint) == 0);
                        }

                        REQUIRE(vircadia_set_my_avatar_joint_flags_count(context, joint_count) == 0);
                        for (int j = 0; j != joint_count; ++j) {
                            auto flags = vircadia_get_avatar_joint_flags(context, avatar, j);
                            REQUIRE(flags != nullptr);
                            REQUIRE(vircadia_set_my_avatar_joint_flags(context, j, *flags) == 0);
                        }

                        auto global_position = vircadia_get_avatar_global_position(context, avatar);
                        REQUIRE(global_position != nullptr);
                        auto offset_position = *global_position;
                        offset_position.x += 1;
                        REQUIRE(vircadia_set_my_avatar_global_position(context, offset_position) == 0);

                        auto orientation = vircadia_get_avatar_orientation(context, avatar);
                        REQUIRE(orientation != nullptr);
                        REQUIRE(vircadia_set_my_avatar_orientation(context, *orientation) == 0);

                        auto scale = vircadia_get_avatar_scale(context, avatar);
                        REQUIRE(scale != nullptr);
                        REQUIRE(vircadia_set_my_avatar_scale(context, *scale) == 0);
                    }
                } else if (receiver != nullptr) {
                    if (std::equal(receiver, receiver + 16, uuid)) {
                        REQUIRE(vircadia_set_my_avatar_global_position(context, global_position) == 0);
                        global_position.x += 0.01f;
                        global_position.y += 0.01f;
                        global_position.z += 0.01f;

                        REQUIRE(vircadia_set_my_avatar_orientation(context, axis_angle({0,1,0},rotation)) == 0);
                        rotation += pi/250;

                        REQUIRE(vircadia_set_my_avatar_scale(context, scale) == 0);
                        scale += 0.001f;

                        auto joint_count = i / 10;
                        REQUIRE(vircadia_set_my_avatar_joint_count(context, joint_count) == 0);
                        for (int j = 0; j != joint_count; ++j) {
                            auto joint = vircadia_vantage{
                                {i/1000.f, i/1000.f, i/1000.f},
                                axis_angle({0,0,1}, pi*i/100.f)
                            };
                            REQUIRE(vircadia_set_my_avatar_joint(context, j, joint) == 0);
                        }
                        receiver_active = true;
                    }
                } else if (starts_with_prefix) {
                    const auto receiver_name = name_prefix + " Receiver";
                    bool starts_with_reveiver = std::search(display_name, display_name + std::strlen(display_name),
                            receiver_name.begin(), receiver_name.end()) == display_name;
                    if (starts_with_reveiver) {
                        receiver = uuid;
                        receiver_active = true;
                        REQUIRE(vircadia_set_my_avatar_display_name(context,
                            (name_prefix + " " + send_string).c_str()) == 0);
                    }
                } else if (i > 100) {
                    REQUIRE(vircadia_set_my_avatar_display_name(context,
                        (name_prefix + " (copying "s + display_name + ")"s).c_str()) == 0);
                    auto skeleton_model_url = vircadia_get_avatar_skeleton_model_url(context, avatar);
                    REQUIRE(skeleton_model_url != nullptr);
                    REQUIRE(vircadia_set_my_avatar_skeleton_model_url(context, skeleton_model_url) == 0);

                    copying = uuid;
                }
            }

            if (receiver != nullptr && !receiver_active) {
                break;
            }

            std::this_thread::sleep_for(10ms);
        }
    }

}
