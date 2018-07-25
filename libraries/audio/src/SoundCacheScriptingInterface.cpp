//
//  SoundCacheScriptingInterface.cpp
//  libraries/audio/src
//
//  Created by David Rowe on 25 Jul 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SoundCacheScriptingInterface.h"

SoundCacheScriptingInterface::SoundCacheScriptingInterface(SoundCache* soundCache) :
    _soundCache(soundCache),
    ScriptableResourceCache::ScriptableResourceCache(soundCache)
{ }

SharedSoundPointer SoundCacheScriptingInterface::getSound(const QUrl& url) {
    return _soundCache->getSound(url);
}
