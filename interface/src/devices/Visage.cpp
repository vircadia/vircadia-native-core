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

Visage::Visage() :
    _active(false) {
#ifdef HAVE_VISAGE
    switchToResourcesParentIfRequired();
    initializeLicenseManager("resources/visage/license.vlc");
    _tracker = new VisageTracker2("resources/visage/Facial Features Tracker - Asymmetric.cfg");
    if (_tracker->trackFromCam()) {
        _data = new FaceData();   
         
    } else {
        delete _tracker;
        _tracker = NULL;
    }
#endif
}

Visage::~Visage() {
#ifdef HAVE_VISAGE
    if (_tracker) {
        _tracker->stop();
        delete _tracker;
        delete _data;
    }
#endif
}

void Visage::update() {
#ifdef HAVE_VISAGE
    _active = (_tracker->getTrackingData(_data) == TRACK_STAT_OK);
    if (!_active) {
        return;
    }
    _headRotation = glm::quat(glm::vec3(_data->faceRotation[0], _data->faceRotation[1], _data->faceRotation[2]));
#endif
}
