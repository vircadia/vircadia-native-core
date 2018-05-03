//
//  SoundCache.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2014-11-13.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SoundCache_h
#define hifi_SoundCache_h

#include <ResourceCache.h>

#include "Sound.h"

/// Scriptable interface for sound loading.
class SoundCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    // Properties are copied over from ResourceCache (see ResourceCache.h for reason).

    /**jsdoc
     * API to manage sound cache resources.
     * @namespace SoundCache
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-server-entity
     * @hifi-assignment-client
     *
     * @property {number} numTotal - Total number of total resources. <em>Read-only.</em>
     * @property {number} numCached - Total number of cached resource. <em>Read-only.</em>
     * @property {number} sizeTotal - Size in bytes of all resources. <em>Read-only.</em>
     * @property {number} sizeCached - Size in bytes of all cached resources. <em>Read-only.</em>
     */


    // Functions are copied over from ResourceCache (see ResourceCache.h for reason).

    /**jsdoc
     * Get the list of all resource URLs.
     * @function SoundCache.getResourceList
     * @returns {string[]}
     */

    /**jsdoc
     * @function SoundCache.dirty
     * @returns {Signal}
     */

    /**jsdoc
     * @function SoundCache.updateTotalSize
     * @param {number} deltaSize
     */

    /**jsdoc
     * Prefetches a resource.
     * @function SoundCache.prefetch
     * @param {string} url - URL of the resource to prefetch.
     * @param {object} [extra=null]
     * @returns {Resource}
     */

    /**jsdoc
     * Asynchronously loads a resource from the specified URL and returns it.
     * @function SoundCache.getResource
     * @param {string} url - URL of the resource to load.
     * @param {string} [fallback=""] - Fallback URL if load of the desired URL fails.
     * @param {} [extra=null]
     * @returns {Resource}
     */


    /**jsdoc 
     * @function SoundCache.getSound
     * @param {string} url
     * @returns {object}
     */
    Q_INVOKABLE SharedSoundPointer getSound(const QUrl& url);
protected:
    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
        const void* extra) override;
private:
    SoundCache(QObject* parent = NULL);
};

#endif // hifi_SoundCache_h
