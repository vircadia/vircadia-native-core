//
//  Algorithms.h
//  libraries/shared/src/shared
//
//  Created by Clement Brisset on 8/9/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Algorithms_h
#define hifi_Algorithms_h

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace alg {

template <class Container, class ValueType>
auto find(const Container& container, const ValueType& value) -> decltype(std::begin(container)) {
    return std::find(std::begin(container), std::end(container), value);
}

template <class Container, class Predicate>
auto find_if(const Container& container, Predicate&& predicate) -> decltype(std::begin(container)) {
    return std::find_if(std::begin(container), std::end(container), std::forward<Predicate>(predicate));
}

template <class Container, class Predicate>
auto find_if_not(const Container& container, Predicate&& predicate) -> decltype(std::begin(container)) {
    return std::find_if_not(std::begin(container), std::end(container), std::forward<Predicate>(predicate));
}

}

#endif // hifi_Algorithms_hpp
