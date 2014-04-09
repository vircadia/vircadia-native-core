//
//  Faceplus.cpp
//  interface
//
//  Created by Andrzej Kapolka on 4/8/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifdef HAVE_FACEPLUS
#include <faceplus.h>
#endif

#include "Faceplus.h"

Faceplus::Faceplus() :
    _active(false) {
}

void Faceplus::init() {
#ifdef HAVE_FACEPLUS
    // these are ignored--any values will do
    faceplus_log_in("username", "password");
#endif
}

void Faceplus::update() {
}

void Faceplus::reset() {
}


