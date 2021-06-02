//
//  TextureCacheScriptingInterface.h
//  libraries/mmodel-networking/src/model-networking
//
//  Created by David Rowe on 25 Jul 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#ifndef hifi_TextureCacheScriptingInterface_h
#define hifi_TextureCacheScriptingInterface_h

#include <QObject>

#include <ResourceCache.h>

#include "TextureCache.h"

class TextureCacheScriptingInterface : public ScriptableResourceCache, public Dependency {
    Q_OBJECT

    // Properties are copied over from ResourceCache (see ResourceCache.h for reason).

    /*@jsdoc
     * The <code>TextureCache</code> API manages texture cache resources.
     *
     * @namespace TextureCache
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {number} numTotal - Total number of total resources. <em>Read-only.</em>
     * @property {number} numCached - Total number of cached resource. <em>Read-only.</em>
     * @property {number} sizeTotal - Size in bytes of all resources. <em>Read-only.</em>
     * @property {number} sizeCached - Size in bytes of all cached resources. <em>Read-only.</em>
     * @property {number} numGlobalQueriesPending - Total number of global queries pending (across all resource cache managers).
     *     <em>Read-only.</em>
     * @property {number} numGlobalQueriesLoading - Total number of global queries loading (across all resource cache managers).
     *     <em>Read-only.</em>
     *
     * @borrows ResourceCache.getResourceList as getResourceList
     * @borrows ResourceCache.updateTotalSize as updateTotalSize
     * @borrows ResourceCache.prefetch as prefetch
     * @borrows ResourceCache.dirty as dirty
     */

public:
    TextureCacheScriptingInterface();

    /*@jsdoc
     * Prefetches a texture resource of specific type.
     * @function TextureCache.prefetch
     * @variation 0
     * @param {string} url - The URL of the texture to prefetch.
     * @param {TextureCache.TextureType} type - The type of the texture.
     * @param {number} [maxNumPixels=67108864] - The maximum number of pixels to use for the image. If the texture has more 
     *     than this number it is downscaled.
     * @returns {ResourceObject} A resource object.
     */
    Q_INVOKABLE ScriptableResource* prefetch(const QUrl& url, int type, int maxNumPixels = ABSOLUTE_MAX_TEXTURE_NUM_PIXELS);

signals:
    /*@jsdoc
     * @function TextureCache.spectatorCameraFramebufferReset
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void spectatorCameraFramebufferReset();
};

#endif // hifi_TextureCacheScriptingInterface_h
