//
//  Hair.h
//  interface/src
//
//  Created by Philip on June 26, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Hair_h
#define hifi_Hair_h

#include <iostream>

#include <glm/glm.hpp>
#include <SharedUtil.h>

#include "GeometryUtil.h"
#include "InterfaceConfig.h"
#include "Util.h"


class Hair {
public:
    Hair();
    void simulate(float deltaTime);
    void render();
    
private:
    
   };

#endif // hifi_Hair_h
