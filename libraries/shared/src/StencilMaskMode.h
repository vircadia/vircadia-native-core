//
//  Created by Sam Gondelman on 3/26/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StencilMaskMode_h
#define hifi_StencilMaskMode_h

enum class StencilMaskMode {
    NONE = -1,  // for legacy reasons, this is -1
    PAINT = 0,
    MESH = 1
};

#endif  // hifi_StencilMaskMode_h