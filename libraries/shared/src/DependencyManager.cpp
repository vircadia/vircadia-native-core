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

#include "SharedUtil.h"
#include "Finally.h"

static const char* const DEPENDENCY_PROPERTY_NAME = "com.highfidelity.DependencyMananger";

DependencyManager& DependencyManager::manager() {
    static DependencyManager* instance = globalInstance<DependencyManager>(DEPENDENCY_PROPERTY_NAME);
    return *instance;
}

QSharedPointer<Dependency>& DependencyManager::safeGet(size_t hashCode) {
    return _instanceHash[hashCode];
}
