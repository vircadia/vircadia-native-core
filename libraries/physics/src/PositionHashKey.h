//
//  PositionHashKey.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PositionHashKey_h
#define hifi_PositionHashKey_h

#include <glm/glm.hpp>

#include <SharedUtil.h>

#include "DoubleHashKey.h"

class PositionHashKey : public DoubleHashKey {
public:
    static int computeHash(const glm::vec3& center);
    static int computeHash2(const glm::vec3& center);
    PositionHashKey(glm::vec3 center);
};

#endif // hifi_PositionHashKey_h
