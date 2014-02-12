//
//  Visage.cpp
//  interface
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifdef HAVE_VISAGE
#include <VisageTracker2.h>
#endif

#include <SharedUtil.h>

#include "Visage.h"

namespace VisageSDK {
    void initializeLicenseManager(char* licenseKeyFileName);
}

using namespace VisageSDK;

Visage::Visage() {
#ifdef HAVE_VISAGE
    switchToResourcesParentIfRequired();
    initializeLicenseManager("resources/visage/license.vlc");
    _tracker = new VisageTracker2("resources/visage/Facial Features Tracker - Asymmetric.cfg");
    _tracker->trackFromCam();
#endif
}

Visage::~Visage() {
#ifdef HAVE_VISAGE
    delete _tracker;
#endif
}
