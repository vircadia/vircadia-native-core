//
//  AnimationCacheScriptingInterface.h
//  libraries/animation/src
//
//  Created by David Rowe on 25 Jul 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#ifndef hifi_AnimationCacheScriptingInterface_h
#define hifi_AnimationCacheScriptingInterface_h

#include <QObject>

#include <ResourceCache.h>

#include "AnimationCache.h"

class AnimationCacheScriptingInterface : public ScriptableResourceCache, public Dependency {
    Q_OBJECT

    // Properties are copied over from ResourceCache (see ResourceCache.h for reason).

    /*@jsdoc
     * The <code>AnimationCache</code> API manages animation cache resources.
     *
     * @namespace AnimationCache
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     * @hifi-assignment-client
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
    AnimationCacheScriptingInterface();

    /*@jsdoc
     * Gets information about an animation resource.
     * @function AnimationCache.getAnimation
     * @param {string} url - The URL of the animation.
     * @returns {AnimationObject} An animation object.
     */
    Q_INVOKABLE AnimationPointer getAnimation(const QString& url);
};

#endif // hifi_AnimationCacheScriptingInterface_h
