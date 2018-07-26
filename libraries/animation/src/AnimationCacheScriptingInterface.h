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

    /**jsdoc
     * API to manage animation cache resources.
     * @namespace AnimationCache
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-assignment-client
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
    AnimationCacheScriptingInterface();

    /**jsdoc
     * Returns animation resource for particular animation.
     * @function AnimationCache.getAnimation
     * @param {string} url - URL to load.
     * @returns {AnimationObject} animation
     */
    Q_INVOKABLE AnimationPointer getAnimation(const QString& url);
};

#endif // hifi_AnimationCacheScriptingInterface_h
