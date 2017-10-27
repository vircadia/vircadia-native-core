//
//  Created by Bradley Austin Davis on 2015/12/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceScriptingInterface.h"

#include "ResourceManager.h"

void ResourceScriptingInterface::overrideUrlPrefix(const QString& prefix, const QString& replacement) {
    DependencyManager::get<ResourceManager>()->setUrlPrefixOverride(prefix, replacement);
}
