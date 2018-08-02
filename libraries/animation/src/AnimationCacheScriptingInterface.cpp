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

AnimationCacheScriptingInterface::AnimationCacheScriptingInterface() :
    ScriptableResourceCache::ScriptableResourceCache(DependencyManager::get<AnimationCache>())
{ }

AnimationPointer AnimationCacheScriptingInterface::getAnimation(const QString& url) {
    return DependencyManager::get<AnimationCache>()->getAnimation(QUrl(url));
}
