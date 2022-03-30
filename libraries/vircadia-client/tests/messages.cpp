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

#include "../src/client.h"
#include "../src/messages.h"
#include "../src/message_types.h"
#include "../src/error.h"

#include <thread>
#include <chrono>

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
                REQUIRE(message != nullptr);
                if (message == test_message) {
                    message_received = true;
                }
            }

            if (!message_sent) {
                REQUIRE(vircadia_send_message(context, text_messages, test_channel.c_str(), test_message.c_str(), false) == 0);
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

}
