//
//  AnimationCacheScriptingInterface.cpp
//  libraries/animation/src
//
//  Created by David Rowe on 25 Jul 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimationCacheScriptingInterface.h"

AnimationCacheScriptingInterface::AnimationCacheScriptingInterface(AnimationCache* animationCache) :
    _animationCache(animationCache),
    ScriptableResourceCache::ScriptableResourceCache(animationCache)
{ }

AnimationPointer AnimationCacheScriptingInterface::getAnimation(const QUrl& url) {
    return _animationCache->getAnimation(url);
}