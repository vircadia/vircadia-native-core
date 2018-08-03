//
//  SoundCacheScriptingInterface.h
//  libraries/audio/src
//
//  Created by David Rowe on 25 Jul 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#ifndef hifi_SoundCacheScriptingInterface_h
#define hifi_SoundCacheScriptingInterface_h

#include <QObject>

#include <ResourceCache.h>

#include "SoundCache.h"

class SoundCacheScriptingInterface : public ScriptableResourceCache, public Dependency {
    Q_OBJECT

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
     *
     * @borrows ResourceCache.getResourceList as getResourceList
     * @borrows ResourceCache.updateTotalSize as updateTotalSize
     * @borrows ResourceCache.prefetch as prefetch
     * @borrows ResourceCache.dirty as dirty
     */

public:
    SoundCacheScriptingInterface();

    /**jsdoc
     * Loads the content of an audio file into a {@link SoundObject}, ready for playback by {@link Audio.playSound}.
     * @function SoundCache.getSound
     * @param {string} url - The URL of the audio file to load &mdash; Web, ATP, or file. See {@link SoundObject} for supported 
     *     formats.
     * @returns {SoundObject} The sound ready for playback.
     */
    Q_INVOKABLE SharedSoundPointer getSound(const QUrl& url);
};

#endif // hifi_SoundCacheScriptingInterface_h
