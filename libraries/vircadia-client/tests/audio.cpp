//
//  audio.cpp
//  libraries/vircadia-client/tests
//
//  Created by Nshan G. on 9 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "../src/context.h"
#include "../src/node_list.h"
#include "../src/node_types.h"
#include "../src/audio.h"
#include "../src/audio_constants.h"
#include "../src/error.h"

#include <thread>
#include <chrono>
#include <string>
#include <cstring>

#include <catch2/catch.hpp>

#include "common.h"

using namespace std::literals;

TEST_CASE("Client API audio functionality.", "[client-api-audio]") {

    const float pi = std::acos(-1);

    const int context = vircadia_create_context(vircadia_context_defaults());
    vircadia_connect(context, "localhost");

    {
        std::thread input_thread{};
        std::thread output_thread{};

        auto cleanup = defer([context, &input_thread, &output_thread](){
            if (input_thread.joinable()) {
                input_thread.join();
            }
            if (output_thread.joinable()) {
                output_thread.join();
            }
            REQUIRE(vircadia_destroy_context(context) == 0);
        });


        bool input_started = false;
        uint8_t* input = nullptr;
        bool output_started = false;
        uint8_t* output = nullptr;
        std::vector<float> output_data;

        REQUIRE(vircadia_enable_audio(context) >= 0);
        REQUIRE(vircadia_set_audio_input_echo(context, 1) >= 0);
        REQUIRE(vircadia_set_audio_input_format(context, vircadia_audio_format{
            vircadia_audio_sample_type_float(), 48000, 1}) >= 0);
        REQUIRE(vircadia_set_audio_output_format(context, vircadia_audio_format{
            vircadia_audio_sample_type_float(), 48000, 2}) >= 0);

        for (int i = 0; i < 300; ++i) {
            int status = vircadia_connection_status(context);
            if (status == 1) {

                if (input == nullptr) {
                    input = vircadia_get_audio_input_context(context);
                } else if (!input_started) {
                    input_thread = std::thread([input, pi, current_sample = 0]() mutable {
                        constexpr int sample_size = 480;
                        static float data[sample_size];
                        while (current_sample != 48000) {
                            for (int i = 0; i < sample_size; ++i) {
                                data[i] = std::sin(148 * 2 * pi * ((current_sample%48000)/48000.f));
                                ++current_sample;
                            }
                            REQUIRE(vircadia_set_audio_input_data(input,
                                reinterpret_cast<const uint8_t*>(data), sample_size * sizeof(float)) >= 0);
                            std::this_thread::sleep_for(10ms);
                        }
                    });
                    input_started = true;
                }

                if (output == nullptr) {
                    output = vircadia_get_audio_output_context(context);
                } else if(!output_started) {
                    output_thread = std::thread([output, &output_data]() {
                        constexpr int sample_size = 480 * 2;
                        while (output_data.size() != 48000 * 2) {
                            output_data.resize(output_data.size() + sample_size);
                            REQUIRE(vircadia_get_audio_output_data(output,
                                reinterpret_cast<uint8_t*>(output_data.data() + output_data.size() - sample_size),
                                sample_size * sizeof(float)) == sample_size * sizeof(float));
                            std::this_thread::sleep_for(10ms);
                        }
                    });
                    output_started = true;
                }

            }

            std::this_thread::sleep_for(10ms);
        }

        if (!output_data.empty()) {
            // TODO: check this as data arrives and finish the test once satisfied
            REQUIRE(std::count_if(output_data.begin(), output_data.end(),
                [](auto x) { return std::abs(x) > 0.5f; }) > 5000);
        }

    }

}
