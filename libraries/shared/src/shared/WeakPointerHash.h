//
//  WeakPointerHash.h
//  libraries/shared
//
//  Created by Brad Hefta-Gaub on 2/11/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  std::hash<> compatible Hash and Equal functions for std::weak_ptr<> classes
//
//  borrowed heavily from:
//      http://stackoverflow.com/questions/13695640/how-to-make-a-c11-stdunordered-set-of-stdweak-ptr
//
//  warning: 
//      there is some question as to whether or not this hash function is really safe to use because
//      it doesn't guarentee that the hash for the weak_ptr() is the same for every requested hash
//      as noted on the link above, this behavior may cause instability or undefined behavior 
//      in some hash users when the weak_ptr<> has expired.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WeakPointerHash_h
#define hifi_WeakPointerHash_h

#include <functional>
#include <memory>

template<typename T>
struct WeakPointerHash : public std::unary_function<std::weak_ptr<T>, size_t> {
    size_t operator()(const std::weak_ptr<T>& weakPointer) {
        auto sharedPointer = weakPointer.lock();
        return std::hash<decltype(sharedPointer)>()(sharedPointer);
    }
};

template<typename T>
struct WeakPointerEqual : public std::unary_function<std::weak_ptr<T>, bool> {
    bool operator()(const std::weak_ptr<T>& left, const std::weak_ptr<T>& right) {
        return !left.owner_before(right) && !right.owner_before(left);
    }
};

#endif // hifi_WeakPointerHash_h