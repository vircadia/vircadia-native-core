//
//  OpenEXRReader.h
//  image/src/image
//
//  Created by Olivier Prat
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_image_OpenEXRReader_h
#define hifi_image_OpenEXRReader_h

#include "Image.h"

namespace image {

    // TODO Move this into a plugin that QImageReader can use
    Image readOpenEXR(QIODevice& contents, const std::string& filename);

}

#endif // hifi_image_OpenEXRReader_h
