//
//  TextureCacheScriptingInterface.cpp
//  libraries/mmodel-networking/src/model-networking
//
//  Created by David Rowe on 25 Jul 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextureCacheScriptingInterface.h"

TextureCacheScriptingInterface::TextureCacheScriptingInterface(TextureCache* textureCache) :
    _textureCache(textureCache),
    ScriptableResourceCache::ScriptableResourceCache(textureCache)
{
    connect(_textureCache, &TextureCache::spectatorCameraFramebufferReset, 
        this, &TextureCacheScriptingInterface::spectatorCameraFramebufferReset);
}

ScriptableResource* TextureCacheScriptingInterface::prefetch(const QUrl& url, int type, int maxNumPixels) {
    return _textureCache->prefetch(url, type, maxNumPixels);
}
