//
//  Audio.h
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 28/5/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_scripting_Audio_h
#define hifi_scripting_Audio_h

#include "AudioScriptingInterface.h"

namespace scripting {

class Audio : public AudioScriptingInterface {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    // TODO: Q_PROPERTY(bool mute)
    // TODO: Q_PROPERTY(bool noiseReduction)
    // TODO: Q_PROPERTY(bool showMicLevel)
    // TODO: Q_PROPERTY(? devices)

public:
    virtual ~Audio() {}

protected:
    Audio() {}
};

};

#endif // hifi_scripting_Audio_h
