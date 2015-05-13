//
//  Created by Bradley Austin Davis 2015/05/13
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AbstractViewStateInterface.h"

static AbstractViewStateInterface* INSTANCE{nullptr};

AbstractViewStateInterface* AbstractViewStateInterface::instance() {
    return INSTANCE;
}

void AbstractViewStateInterface::setInstance(AbstractViewStateInterface* instance) {
    INSTANCE = instance;
}

