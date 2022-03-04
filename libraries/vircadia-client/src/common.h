//
//  common.h
//  libraries/client/src
//
//  Created by Nshan G. on 1 March 2022.
//  Copyright 2022 Vircadia contributors.
//

#ifndef VIRCADIA_LIBRARIES_CLIENT_SRC_COMMON_H
#define VIRCADIA_LIBRARIES_CLIENT_SRC_COMMON_H

#if defined(_MSC_VER)
#ifdef VIRCADIA_CLIENT_DLL_EXPORT
/* We are building this library */
#   define VIRCADIA_CLIENT_DYN_API extern "C" __declspec(dllexport)
#else
/* We are using this library */
#   define VIRCADIA_CLIENT_DYN_API  extern "C" __declspec(dllimport)
#endif
#else
    #define VIRCADIA_CLIENT_DYN_API extern "C"
#endif

#endif /* end of include guard */
