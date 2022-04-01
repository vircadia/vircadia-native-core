//
//  messages.cpp
//  libraries/client/tests
//
//  Created by Nshan G. on 30 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "../src/context.h"
#include "../src/node_list.h"
#include "../src/messages.h"
#include "../src/message_types.h"
#include "../src/error.h"

#include <thread>
#include <chrono>
#include <string>
#include <cstring>

#include <catch2/catch.hpp>

using namespace std::literals;

TEST_CASE("Client API messaging functionality.", "[client-api-messaging]") {
    const std::string test_channel = "Chat";
    const std::string test_message = "{ \"message\": \"This is vircadia client library unit test speaking.\", \"displayName\": \"client_unit_test\", \"type\":\"TransmitChatMessage\", \"channel\": \"Domain\" }";

    const int context = vircadia_create_context(vircadia_context_defaults());
    vircadia_connect(context, "localhost");

    const int text_messages = vircadia_text_messages();

    bool message_received = false;
    bool message_sent = false;
    for (int i = 0; i < 100; ++i) {
        int status = vircadia_connection_status(context);
        if (status == 1) {
            REQUIRE(vircadia_enable_messages(context, text_messages) == 0);

            REQUIRE(vircadia_messages_subscribe(context, test_channel.c_str()) == 0);

            REQUIRE(vircadia_update_messages(context, text_messages) == 0);
            int count = vircadia_messages_count(context, text_messages);
            REQUIRE(count >= 0);
            for (int i = 0; i != count; ++i) {
                auto message = vircadia_get_message(context, text_messages, i);
                auto message_size = vircadia_get_message_size(context, text_messages, i);
                auto channel = vircadia_get_message_channel(context, text_messages, i);
                auto sender = vircadia_get_message_sender(context, text_messages, i);
                REQUIRE(vircadia_is_message_local_only(context, text_messages, i) == 0);
                REQUIRE(message != nullptr);
                REQUIRE(message_size >= 0);
                REQUIRE(std::size_t(message_size) == std::strlen(message));
                REQUIRE(channel != nullptr);
                REQUIRE(sender != nullptr);
                if (message == test_message && channel == test_channel) {
                    message_received = true;
                }
                REQUIRE(vircadia_get_message(context, vircadia_data_messages(), i) == nullptr);
                REQUIRE(vircadia_is_message_local_only(context, vircadia_data_messages(), i) == vircadia_error_message_type_disabled());
                REQUIRE(vircadia_get_message_size(context, vircadia_data_messages(), i) == vircadia_error_message_type_disabled());
            }
            REQUIRE(vircadia_messages_count(context, vircadia_data_messages()) == vircadia_error_message_type_disabled());

            REQUIRE(vircadia_get_message(context, text_messages, count) == nullptr);
            REQUIRE(vircadia_is_message_local_only(context, text_messages, count) == vircadia_error_message_invalid());

            if (!message_sent) {
                REQUIRE(vircadia_send_message(context, text_messages, test_channel.c_str(), test_message.c_str(), -1, false) == 0);
                message_sent = true;;
            }

            REQUIRE(vircadia_clear_messages(context, text_messages) == 0);

            if (message_received) {
                break;
            }
        }

        std::this_thread::sleep_for(100ms);
    }

    if (message_sent) {
        REQUIRE(message_received);
    }

    REQUIRE(vircadia_destroy_context(context) == 0);

    REQUIRE(vircadia_enable_messages(context, text_messages) == vircadia_error_context_invalid());
    REQUIRE(vircadia_messages_subscribe(context, test_channel.c_str()) == vircadia_error_context_invalid());
    REQUIRE(vircadia_send_message(context, text_messages, test_channel.c_str(), test_message.c_str(), -1, false) == vircadia_error_context_invalid());

}
