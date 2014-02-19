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

const glm::vec3 DEFAULT_HEAD_ORIGIN(0.0f, 0.0f, 0.7f);

Visage::Visage() :
    _active(false),
    _headOrigin(DEFAULT_HEAD_ORIGIN),
    _estimatedEyePitch(0.0f),
    _estimatedEyeYaw(0.0f) {
#ifdef HAVE_VISAGE
    switchToResourcesParentIfRequired();
    QByteArray licensePath = "resources/visage/license.vlc";
    initializeLicenseManager(licensePath.data());
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

const float TRANSLATION_SCALE = 20.0f;

void Visage::update() {
#ifdef HAVE_VISAGE
    _active = (_tracker && _tracker->getTrackingData(_data) == TRACK_STAT_OK);
    if (!_active) {
        return;
    }
    _headRotation = glm::quat(glm::vec3(-_data->faceRotation[0], -_data->faceRotation[1], _data->faceRotation[2]));    
    _headTranslation = (glm::vec3(_data->faceTranslation[0], _data->faceTranslation[1], _data->faceTranslation[2]) -
        _headOrigin) * TRANSLATION_SCALE;
    _estimatedEyePitch = glm::degrees(-_data->gazeDirection[1]);
    _estimatedEyeYaw = glm::degrees(-_data->gazeDirection[0]);
    
    for (int i = 0; i < _data->actionUnitCount; i++) {
        if (!_data->actionUnitsUsed[i]) {
            continue;
        }
        qDebug() << _data->actionUnitsNames[i] << _data->actionUnits[i];
    }
#endif
}

void Visage::reset() {
    _headOrigin += _headTranslation / TRANSLATION_SCALE;
}
