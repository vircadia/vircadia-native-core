//
//  Created by Bradley Austin Davis on 2016/11/29
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GlobalAppProperties.h"

namespace hifi { namespace properties {

    const char* CRASHED = "com.highfidelity.crashed";
    const char* STEAM = "com.highfidelity.launchedFromSteam";
    const char* LOGGER = "com.highfidelity.logger";

    namespace gl {
        const char* BACKEND = "com.highfidelity.gl.backend";
        const char* MAKE_PROGRAM_CALLBACK = "com.highfidelity.gl.makeProgram";
        const char* PRIMARY_CONTEXT = "com.highfidelity.gl.primaryContext";
    }

} }
