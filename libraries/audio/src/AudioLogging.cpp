//
//  AudioLogging.cpp
//  libraries/audio/src
//
//  Created by Seth Alves on 4/6/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioLogging.h"

Q_LOGGING_CATEGORY(audio, "hifi.audio")

#if DEV_BUILD || PR_BUILD
Q_LOGGING_CATEGORY(audiostream, "hifi.audio-stream", QtInfoMsg)
#else
Q_LOGGING_CATEGORY(audiostream, "hifi.audio-stream", QtWarningMsg)
#endif
