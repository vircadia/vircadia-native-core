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

    /**jsdoc
     * API to manage texture cache resources.
     * @namespace TextureCache
     *
     * @hifi-interface
     * @hifi-client-entity
     *
     * @property {number} numTotal - Total number of total resources. <em>Read-only.</em>
     * @property {number} numCached - Total number of cached resource. <em>Read-only.</em>
     * @property {number} sizeTotal - Size in bytes of all resources. <em>Read-only.</em>
     * @property {number} sizeCached - Size in bytes of all cached resources. <em>Read-only.</em>
     *
     * @borrows ResourceCache.getResourceList as getResourceList
     * @borrows ResourceCache.updateTotalSize as updateTotalSize
     * @borrows ResourceCache.prefetch as prefetch
     * @borrows ResourceCache.dirty as dirty
     */

public:
    TextureCacheScriptingInterface();

    /**jsdoc
     * @function TextureCache.prefetch
     * @param {string} url
     * @param {number} type
     * @param {number} [maxNumPixels=67108864]
     * @returns {ResourceObject}
     */
    Q_INVOKABLE ScriptableResource* prefetch(const QUrl& url, int type, int maxNumPixels = ABSOLUTE_MAX_TEXTURE_NUM_PIXELS);

signals:
    /**jsdoc
     * @function TextureCache.spectatorCameraFramebufferReset
     * @returns {Signal}
     */
    void spectatorCameraFramebufferReset();
};

#endif // hifi_TextureCacheScriptingInterface_h
