//
//  Created by Sam Gondelman on 7/13/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <plugins/Plugin.h>

class InputPlugin : public Plugin {
    Q_OBJECT
public:
    virtual bool isHandController() = 0;
};

