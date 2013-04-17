//
// starfield/Config.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__Config__
#define __interface__starfield__Config__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

//
// Compile time configuration:
//

#ifndef STARFIELD_HEMISPHERE_ONLY
#define STARFIELD_HEMISPHERE_ONLY 0 // set to 1 for hemisphere only
#endif

#ifndef STARFIELD_LOW_MEMORY
#define STARFIELD_LOW_MEMORY 0 //  set to 1 not to use 16-bit types
#endif

#ifndef STARFIELD_DEBUG_LOD
#define STARFIELD_DEBUG_LOD 0 // set to 1 to peek behind the scenes
#endif

#ifndef STARFIELD_MULTITHREADING
#define STARFIELD_MULTITHREADING 0
#endif

//
// Dependencies:
//

#include "InterfaceConfig.h"
#include "OGlProgram.h"
#include "Log.h"

#include <cstddef>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cctype>

#include <stdint.h>

#if STARFIELD_MULTITHREADING
#include <mutex>
#include <atomic>
#endif

#include <new>
#include <vector>
#include <memory>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/swizzle.hpp>

#include "UrlReader.h"
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
    using glm::swizzle;
    using glm::X;
    using glm::Y;
    using glm::Z;
    using glm::W;
    using glm::mat4;
    using glm::column;
    using glm::row;

    using namespace std;


#if STARFIELD_SAVE_MEMORY
    typedef uint16_t nuint;
    typedef uint32_t wuint;
#else
    typedef uint32_t nuint;
    typedef uint64_t wuint;
#endif

}

#endif

