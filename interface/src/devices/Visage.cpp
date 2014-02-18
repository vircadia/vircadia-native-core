//
//  Visage.cpp
//  interface
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <SharedUtil.h>

#ifdef HAVE_VISAGE
#include <VisageTracker2.h>
#endif

#include "Visage.h"

namespace VisageSDK {
#ifdef WIN32
    void __declspec(dllimport) initializeLicenseManager(char* licenseKeyFileName);
#else
	void initializeLicenseManager(char* licenseKeyFileName);
#endif
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
    _tracker->stop();
    delete _tracker;
#endif
}
