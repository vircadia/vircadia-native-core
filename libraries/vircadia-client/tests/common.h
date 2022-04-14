//
//  common.h
//  libraries/vircadia-client/tests
//
//  Created by Nshan G. on 13 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_TESTS_COMMON_H
#define LIBRARIES_VIRCADIA_CLIENT_TESTS_COMMON_H

#include <memory>

using deferred = std::shared_ptr<void>;

template <typename Cleanup>
auto defer(Cleanup cleanup) {
    return std::shared_ptr<void>(nullptr,
        [cleanup = std::move(cleanup)] (void*) { cleanup(); });
}

#endif /* end of include guard */
