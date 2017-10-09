//
//  androidhacks.h
//  interface/src
//
//  Created by Cristian Duarte & Gabriel Calero on 1/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  hacks to get android to compile
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#pragma once
#ifndef hifi_shared_platform_androidhacks_h
#define hifi_shared_platform_androidhacks_h

#include <string>
#include <sstream>

#include <ciso646>

// Only for gnu stl, so checking if using llvm
// (If there is a better check than this http://stackoverflow.com/questions/31657499/how-to-detect-stdlib-libc-in-the-preprocessor, improve this one)
#if (_LIBCPP_VERSION)
    // NOTHING, these functions are well defined on libc++
#else

using namespace std;
namespace std
{
                // to_string impl
                // error: no member named 'to_string' in namespace 'std'
                // http://stackoverflow.com/questions/26095886/error-to-string-is-not-a-member-of-std
    template <typename T>
    inline std::string to_string(T value) {
        std::ostringstream os;
        os << value;
        return os.str();
    }

    inline float stof(std::string str) {
                return atof(str.c_str());
    }
}

#endif // _LIBCPP_VERSION

#endif // hifi_androidhacks_h
