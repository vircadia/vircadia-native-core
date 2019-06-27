//
//  WebRTC.h
//  libraries/shared/src/shared/
//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebRTC_h
#define hifi_WebRTC_h

#if defined(Q_OS_MAC)
#  define WEBRTC_ENABLED 1
#  define WEBRTC_POSIX 1
#elif defined(WIN32)
#  define WEBRTC_ENABLED 1
#  define WEBRTC_WIN 1
#  define NOMINMAX 1
#  define WIN32_LEAN_AND_MEAN 1
#elif defined(Q_OS_ANDROID)
// no webrtc for android -- this is here so the LINUX clause doesn't get used, below
#elif defined(Q_OS_LINUX)
#  define WEBRTC_ENABLED 1
#  define WEBRTC_POSIX 1
#endif

#if defined(WEBRTC_ENABLED)
#  include <modules/audio_processing/include/audio_processing.h>
#  include "modules/audio_processing/audio_processing_impl.h"
#endif

#endif // hifi_WebRTC_h
