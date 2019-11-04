//
//  MaterialCacheScriptingInterface.cpp
//
//  Created by Sam Gateau on 17 September 2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MaterialCacheScriptingInterface.h"

MaterialCacheScriptingInterface::MaterialCacheScriptingInterface() :
    ScriptableResourceCache::ScriptableResourceCache(DependencyManager::get<MaterialCache>())
{ }

