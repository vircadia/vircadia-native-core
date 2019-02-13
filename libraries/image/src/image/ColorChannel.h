//
//  ColorChannel.h
//  libraries/image/src/image
//
//  Created by Sabrina Shanman on 2019/02/12.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_image_ColorChannel_h
#define hifi_image_ColorChannel_h

namespace image {
    enum class ColorChannel {
        NONE,
        RED,
        GREEN,
        BLUE,
        ALPHA,
        COUNT
    };
};

#endif // hifi_image_ColorChannel_h
