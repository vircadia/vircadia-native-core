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

/**jsdoc
 * API to manage Sound Cache resources
 * @namespace SoundCache
 */


/// Scriptable interface for sound loading.
class SoundCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    // Copied over from ResourceCache (see ResourceCache.h for reason)

    /**jsdoc
     * @namespace SoundCache
     * @property numTotal {number} total number of total resources
     * @property numCached {number} total number of cached resource
     * @property sizeTotal {number} size in bytes of all resources
     * @property sizeCached {number} size in bytes of all cached resources
     */

    /**jsdoc
     * Returns the total number of resources
     * @function SoundCache.getNumTotalResources
     * @returns {number}
     */

    /**jsdoc
     * Returns the total size in bytes of all resources
     * @function SoundCache.getSizeTotalResources
     * @returns {number}
     */

    /**jsdoc
     * Returns the total number of cached resources
     * @function SoundCache.getNumCachedResources
     * @returns {number}
     */

    /**jsdoc
     * Returns the total size in bytes of cached resources
     * @function SoundCache.getSizeCachedResources
     * @returns {number}
     */

    /**jsdoc
     * Returns list of all resource urls
     * @function SoundCache.getResourceList
     * @returns {string[]}
     */

    /**jsdoc
     * Returns animation resource for particular animation
     * @function SoundCache.getAnimation
     * @param url {string} url to load
     * @returns {Resource} animation
     */

    /**jsdoc
     * Asynchronously loads a resource from the spedified URL and returns it.
     * @param url {string} url of resource to load
     * @param fallback {string} fallback URL if load of the desired url fails
     * @function SoundCache.getResource
     * @returns {Resource}
     */

    /**jsdoc
     * Prefetches a resource.
     * @param url {string} url of resource to load
     * @function SoundCache.prefetch
     * @returns {Resource}
     */

    /**jsdoc
     * @param {number} deltaSize
     * @function SoundCache.updateTotalSize
     * @returns {Resource}
     */

    /**jsdoc
     * @function SoundCache.dirty
     * @returns {Signal}
     */

    /**jsdoc 
     * @function SoundCache.getSound
     * @param {string} url
     * @returns {}
     */

    Q_INVOKABLE SharedSoundPointer getSound(const QUrl& url);
protected:
    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
        const void* extra) override;
private:
    SoundCache(QObject* parent = NULL);
};

#endif // hifi_SoundCache_h
