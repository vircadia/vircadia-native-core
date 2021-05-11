//
//  ProceduralMaterialCacheScriptingInterface.h
//
//  Created by Sam Gateau on 17 September 2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#ifndef hifi_ProceduralMaterialCacheScriptingInterface_h
#define hifi_ProceduralMaterialCacheScriptingInterface_h

#include <QObject>

#include <ResourceCache.h>

#include "ProceduralMaterialCache.h"

class MaterialCacheScriptingInterface : public ScriptableResourceCache, public Dependency {
    Q_OBJECT

    // Properties are copied over from ResourceCache (see ResourceCache.h for reason).

    /*@jsdoc
     * The <code>MaterialCache</code> API manages material cache resources.
     *
     * @namespace MaterialCache
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
    MaterialCacheScriptingInterface();
};

#endif // hifi_ProceduralMaterialCacheScriptingInterface_h
