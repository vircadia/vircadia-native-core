//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "NotConditional.h"

using namespace controller;

bool NotConditional::satisfied() {
    if (_operand) {
        return (!_operand->satisfied());
    } else {
        return false;
    }
}

