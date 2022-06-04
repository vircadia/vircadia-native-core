//
//  avatars_control.cpp
//  libraries/vircadia-client/tests
//
//  Created by Nshan G. on 3 June 2022.
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
#include <optional>

#include <catch2/catch.hpp>

#include "common.h"

bool operator<(vircadia_vector a, vircadia_vector b) {
    return a.x < b.x && a.y < b.y && a.z < b.z;
}

bool operator!=(vircadia_quaternion a, vircadia_quaternion b) {
    return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.y;
}

using namespace std::literals;

TEST_CASE("Client API avatar functionality - receiver.", "[client-api-avatars-receiver]") {

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

    const int context = vircadia_create_context(vircadia_context_defaults());
    vircadia_connect(context, "localhost");

    std::optional<vircadia_vector> last_global_position {};
    std::optional<vircadia_quaternion> last_orientation {};
    std::optional<float> last_scale{};
    std::optional<int> last_joint_count{};
    std::optional<vircadia_vantage> last_joint_value{};
    bool position_increased = false;
    bool orientation_changed = false;
    bool scale_increased = false;
    bool joint_count_increased = false;
    bool joint_position_increased = false;
    bool joint_rotation_changed = false;

    const uint8_t* sender = nullptr;
    const uint8_t* session_uuid = nullptr;

    {
        auto cleanup = defer([context](){
            REQUIRE(vircadia_destroy_context(context) == 0);
        });

        REQUIRE(vircadia_enable_avatars(context) == 0);
        REQUIRE(vircadia_set_my_avatar_display_name(context, (name_prefix + " Receiver").c_str()) == 0);


        for (int i = 0; i < 1000; ++i) {

            REQUIRE(vircadia_update_nodes(context) == 0);
            session_uuid = vircadia_client_get_session_uuid(context);
            REQUIRE(session_uuid != nullptr);

            REQUIRE(vircadia_update_avatars(context) == 0);

            auto avatar_count = vircadia_get_avatar_count(context);
            REQUIRE(avatar_count >= 0);

            for (int avatar = 0; avatar != avatar_count; ++avatar) {
                const uint8_t* uuid = vircadia_get_avatar_uuid(context, avatar);
                REQUIRE(uuid != nullptr);
                auto display_name = vircadia_get_avatar_display_name(context, avatar);
                REQUIRE(display_name != nullptr);

                if (sender != nullptr) {
                    if (std::equal(uuid, uuid + 16, sender)) {
                        const char* model = vircadia_get_avatar_skeleton_model_url(context, avatar);
                        REQUIRE(model != nullptr);
                        REQUIRE(std::any_of(avatar_urls.begin(), avatar_urls.end(),
                            [model = std::string(model)](auto url) { return model == url; }));

                        const auto bounding_box = vircadia_get_avatar_bounding_box(context, avatar);
                        REQUIRE(bounding_box != nullptr);
                        REQUIRE(bounding_box->dimensions.x == 1);
                        REQUIRE(bounding_box->dimensions.y == 1);
                        REQUIRE(bounding_box->dimensions.z == 1);
                        REQUIRE(bounding_box->offset.x == 0);
                        REQUIRE(bounding_box->offset.y == 0);
                        REQUIRE(bounding_box->offset.z == 0);

                        const auto global_position = vircadia_get_avatar_global_position(context, avatar);
                        REQUIRE(global_position != nullptr);
                        if (!last_global_position) {
                            last_global_position = *global_position;
                        } else if(*last_global_position < *global_position) {
                            position_increased = true;
                        }

                        const auto orientation = vircadia_get_avatar_orientation(context, avatar);
                        REQUIRE(orientation != nullptr);
                        if (!last_orientation) {
                            last_orientation = *orientation;
                        } else if(*last_orientation != *orientation) {
                            orientation_changed = true;
                        }

                        const auto scale = vircadia_get_avatar_scale(context, avatar);
                        REQUIRE(scale != nullptr);
                        if (!last_scale) {
                            last_scale = *scale;
                        } else if(*last_scale < *scale) {
                            scale_increased = true;
                        }

                        const auto joint_count = vircadia_get_avatar_joint_count(context, avatar);
                        REQUIRE(joint_count >= 0);
                        if (!last_joint_count) {
                            last_joint_count = joint_count;
                        } else if(*last_joint_count < joint_count) {
                            joint_count_increased = true;
                        }

                        if (joint_count > 0) {
                            const auto joint = vircadia_get_avatar_joint(context, avatar, 0);
                            REQUIRE(joint != nullptr);
                            if (!last_joint_value) {
                                last_joint_value = *joint;
                            } else {
                                if (last_joint_value->position < joint->position) {
                                    joint_position_increased = true;
                                }
                                if (last_joint_value->rotation != joint->rotation) {
                                    joint_rotation_changed = true;
                                }
                            }
                        }

                    }
                } else {
                    bool is_me = std::equal(uuid, uuid + 16, session_uuid);
                    auto display_name_end = display_name + std::strlen(display_name);
                    bool starts_with_prefix = std::search(display_name, display_name_end,
                        name_prefix.begin(), name_prefix.end()) != display_name_end;
                    if (!is_me && starts_with_prefix) {
                        bool sending = std::search(display_name, display_name + std::strlen(display_name),
                            send_string.begin(), send_string.end()) != display_name;
                        if (sending) {
                            sender = uuid;
                        }
                    }
                }
            }

            if (position_increased && orientation_changed && scale_increased &&
                joint_count_increased && joint_position_increased && joint_rotation_changed) {
                break;
            }

            std::this_thread::sleep_for(10ms);
        }

        if(sender != nullptr) {
            REQUIRE(position_increased);
            REQUIRE(orientation_changed);
            REQUIRE(scale_increased);
            REQUIRE(joint_count_increased);
            REQUIRE(joint_position_increased);
            REQUIRE(joint_rotation_changed);
        }
    }

}
