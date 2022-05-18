//
//  common.h
//  libraries/vircadia-client/tests
//
//  Created by Nshan G. on 13 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_TESTS_COMMON_H
#define LIBRARIES_VIRCADIA_CLIENT_TESTS_COMMON_H

#include <memory>
#include <array>

using deferred = std::shared_ptr<void>;

template <typename Cleanup>
auto defer(Cleanup cleanup) {
    return std::shared_ptr<void>(nullptr,
        [cleanup = std::move(cleanup)] (void*) { cleanup(); });
}

// cause windows and mac just can't
// https://github.com/vircadia/vircadia/runs/6489707252?check_suite_focus=true
template <typename T, std::size_t N>
struct array : std::array<T,N> {};
template <typename First, typename... Rest>
array(First, Rest...) -> array<First, sizeof...(Rest) + 1>;

#endif /* end of include guard */
