//
//  Error.h
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_ERROR_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_ERROR_H

#include <type_traits>
#include <utility>
#include <algorithm>
#include <functional>
#include <initializer_list>

namespace vircadia::client {

enum class ErrorCode : int {

    CONTEXT_EXISTS,
    CONTEXT_INVALID,
    CONTEXT_LOSS,

    NODE_INVALID,

    MESSAGE_INVALID,
    MESSAGE_TYPE_INVALID,
    MESSAGE_TYPE_DISABLED,

    PACKET_WRITE,

    ARGUMENT_INVALID

};

inline int toInt(ErrorCode code) {
    return -(1 + static_cast<std::underlying_type_t<ErrorCode>>(code));
}

inline bool isError(int result) {
    return result < 0;
}

template <typename T>
bool isError(T* result) {
    return result == nullptr;
}

template <typename R, typename F>
auto chain(R result, F&& f) {
    if (isError(result)) {
        using ReturnType = std::invoke_result_t<F, R>;
        if constexpr (std::is_pointer_v<ReturnType>) {
            return static_cast<ReturnType>(nullptr);
        } else {
            return result;
        }
    } else {
        return std::invoke(std::forward<F>(f), result);
    }
};

template <typename R, typename F>
auto chain(std::initializer_list<R> result, F&& f) {
    auto error = std::find_if(result.begin(), result.end(),
        [](auto&& x) { return isError(x); } );
    if (error != result.end()) {
        using ReturnType = std::invoke_result_t<F, R>;
        if constexpr (std::is_pointer_v<ReturnType>) {
            return static_cast<ReturnType>(nullptr);
        } else {
            return *error;
        }
    } else {
        return std::invoke(std::forward<F>(f), result);
    }
};

template <typename Container>
int checkIndexValid(const Container& container, int index, ErrorCode code) {
    if (index < 0 || index >= static_cast<int>(container.size())) {
        return toInt(code);
    } else {
        return 0;
    }
}

} // namespace vircadia::client

#endif /* end of include guard */
