//
//  version.cpp
//  libraries/client/src
//
//  Created by Nshan G. on 2 Mar 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <../src/version.h>

#include <catch2/catch.hpp>
#include <string>

using namespace std::literals;

TEST_CASE("Sanity checks for client API version information.", "[client-api-version]") {

    auto version = *vircadia_client_version();

    REQUIRE(version.major >= 0);
    REQUIRE(version.minor >= 0);
    REQUIRE(version.tweak >= 0);
    REQUIRE((version.major != 0 || version.minor != 0 || version.tweak != 0));

    REQUIRE(version.commit != ""s);
    REQUIRE(version.number != ""s);
    REQUIRE(version.full != ""s);

    auto number = "v"s + std::to_string(version.major)
        + "."s + std::to_string(version.minor)
        + "."s + std::to_string(version.tweak);

    REQUIRE(version.number == number);
    REQUIRE(version.full == version.number + "-git."s + version.commit);

}
