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

DependencyManager::~DependencyManager() {
    foreach (Dependency* instance, _instanceHash) {
        delete instance;
    }
    _instanceHash.clear();
}