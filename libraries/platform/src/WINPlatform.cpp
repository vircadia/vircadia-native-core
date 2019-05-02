//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WINPlatform.h"

using namespace platform;
   
bool WINInstance::enumerateProcessors() {
    return true;
   
}

std::string WINInstance::getProcessor(int index) {
    
      return "Fake processor";
}


