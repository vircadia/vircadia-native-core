//
//  Created by Bradley Austin Davis on 2016/11/29
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GlobalAppProperties_h
#define hifi_GlobalAppProperties_h

namespace hifi { namespace properties {

    static const char* CRASHED = "com.highfidelity.crashed";
    static const char* STEAM = "com.highfidelity.launchedFromSteam";
    static const char* LOGGER = "com.highfidelity.logger";

    namespace gl {
        static const char* BACKEND = "com.highfidelity.gl.backend";
        static const char* MAKE_PROGRAM_CALLBACK = "com.highfidelity.gl.makeProgram";
        static const char* PRIMARY_CONTEXT = "com.highfidelity.gl.primaryContext";
    }

} }


#endif // hifi_GlobalAppProperties_h
