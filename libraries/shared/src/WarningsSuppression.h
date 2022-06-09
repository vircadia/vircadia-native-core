//
//  WarningsSuppression.h
//
//
//  Created by Dale Glass on 5/6/2022
//  Copyright 2022 Dale Glass
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/*
 * This file provides macros to suppress compile-time warnings.
 * They should be used with extreme caution, only when the compiler is definitely mistaken,
 * when the problem is in third party code that's not practical to patch, or where something
 * is deprecated but can't be dealt with for the time being.
 *
 * Usage of these macros should come with an explanation of why we're using them.
 */


#ifdef OVERTE_WARNINGS_WHITELIST_GCC

    #define OVERTE_IGNORE_DEPRECATED_BEGIN \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")

    #define OVERTE_IGNORE_DEPRECATED_END _Pragma("GCC diagnostic pop")

#elif OVERTE_WARNINGS_WHITELIST_CLANG

    #define OVERTE_IGNORE_DEPRECATED_BEGIN \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")

    #define OVERTE_IGNORE_DEPRECATED_END _Pragma("clang diagnostic pop")

#elif OVERTE_WARNINGS_WHITELIST_MSVC

    #define OVERTE_IGNORE_DEPRECATED_BEGIN \
        _Pragma("warning(push)") \
        _Pragma("warning(disable : 4996)")

    #define OVERTE_IGNORE_DEPRECATED_END _Pragma("warning(pop)")

#else

#warning "Don't know how to suppress warnings on this compiler. Please fix me."

#define OVERTE_IGNORE_DEPRECATED_BEGIN
#define OVERTE_IGNORE_DEPRECATED_END

#endif
