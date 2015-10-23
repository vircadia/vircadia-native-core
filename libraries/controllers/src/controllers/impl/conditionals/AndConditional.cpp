//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AndConditional.h"

using namespace controller;

bool AndConditional::satisfied() {
   for (auto& conditional : _children) {
       if (!conditional->satisfied()) {
           return false;
       }
   }
   return true;
}

