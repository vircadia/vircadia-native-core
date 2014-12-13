//
//  DependencyManager.cpp
//
//
//  Created by Clément Brisset on 12/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DependencyManager.h"

DependencyManager& DependencyManager::getInstance() {
    static DependencyManager instance;
    return instance;
}

DependencyManager::DependencyManager() {
    // Guard against request of ourself
    // We could set the value to this, but since it doesn't make sense to access
    // the DependencyManager instance from the outside let's set it to NULL
    _instanceHash.insert(typeid(DependencyManager).name(), NULL);
}

DependencyManager::~DependencyManager() {
    foreach (Dependency* instance, _instanceHash) {
        if (instance) {
            instance->deleteInstance();
        }
    }
    _instanceHash.clear();
}