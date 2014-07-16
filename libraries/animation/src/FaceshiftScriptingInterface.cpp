//
//  FaceshiftScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ben Arnold on 7/38/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "FaceshiftScriptingInterface.h"

FaceshiftScriptingInterface* FaceshiftScriptingInterface::getInstance() {
    static FaceshiftScriptingInterface sharedInstance;
    return &sharedInstance;
}