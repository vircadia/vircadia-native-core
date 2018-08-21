//
//  Created by Sam Gondelman on 7/20/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BoxBase.h"

QString boxFaceToString(BoxFace face) {
    switch (face) {
        case MIN_X_FACE:
            return "MIN_X_FACE";
        case MAX_X_FACE:
            return "MAX_X_FACE";
        case MIN_Y_FACE:
            return "MIN_Y_FACE";
        case MAX_Y_FACE:
            return "MAX_Y_FACE";
        case MIN_Z_FACE:
            return "MIN_Z_FACE";
        case MAX_Z_FACE:
            return "MAX_Z_FACE";
        default:
            return "UNKNOWN_FACE";
    }
}

BoxFace boxFaceFromString(const QString& face) {
    if (face == "MIN_X_FACE") {
        return MIN_X_FACE;
    } else if (face == "MAX_X_FACE") {
        return MAX_X_FACE;
    } else if (face == "MIN_Y_FACE") {
        return MIN_Y_FACE;
    } else if (face == "MAX_Y_FACE") {
        return MAX_Y_FACE;
    } else if (face == "MIN_Z_FACE") {
        return MIN_Z_FACE;
    } else if (face == "MAX_Z_FACE") {
        return MAX_Z_FACE;
    } else {
        return UNKNOWN_FACE;
    }
}