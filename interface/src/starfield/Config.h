//
//  Config.h
//  interface/src/starfield
//
//  Created by Tobias Schwinger on 3/29/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Config_h
#define hifi_Config_h

#include "InterfaceConfig.h"

#include <cstddef>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cctype>

#include <stdint.h>

#include <new>
#include <vector>
#include <memory>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <ProgramObject.h>

#include "AngleUtil.h"
#include "Radix2InplaceSort.h"
#include "Radix2IntegerScanner.h"
#include "FloodFill.h"

// Namespace configuration:

namespace starfield {

    using glm::vec3;
    using glm::vec4;
    using glm::dot;
    using glm::normalize;
    using glm::mat4;
    using glm::row;

    using namespace std;

    typedef uint32_t nuint;
    typedef quint64 wuint;

}

#endif // hifi_Config_h
