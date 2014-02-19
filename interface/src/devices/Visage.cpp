//
//  Visage.cpp
//  interface
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <QHash>

#include <SharedUtil.h>

#ifdef HAVE_VISAGE
#include <VisageTracker2.h>
#endif

#include "Visage.h"
#include "renderer/FBXReader.h"

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
    _estimatedEyeYaw(0.0f),
    _leftInnerBrowIndex(0),
    _rightInnerBrowIndex(0) {
    
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

static int leftEyeBlinkIndex = 0;
static int rightEyeBlinkIndex = 1;
static int centerBrowIndex = 16;

static QHash<QByteArray, int> createBlendshapeIndices() {
    QHash<QByteArray, QByteArray> blendshapeMap;
    blendshapeMap.insert("Sneer", "au_nose_wrinkler");
    blendshapeMap.insert("JawFwd", "au_jaw_z_push");
    blendshapeMap.insert("JawLeft", "au_jaw_x_push");
    blendshapeMap.insert("JawOpen", "au_jaw_drop");
    blendshapeMap.insert("LipsLowerDown", "au_lower_lip_drop");
    blendshapeMap.insert("LipsUpperUp", "au_upper_lip_raiser");
    blendshapeMap.insert("LipsStretch_L", "au_lip_stretcher_left");
    blendshapeMap.insert("BrowsU_L", "au_left_outer_brow_raiser");
    blendshapeMap.insert("BrowsU_C", "au_left_inner_brow_raiser");
    blendshapeMap.insert("BrowsD_L", "au_left_brow_lowerer");
    blendshapeMap.insert("LipsStretch_R", "au_lip_stretcher_right");
    blendshapeMap.insert("BrowsU_R", "au_right_outer_brow_raiser");
    blendshapeMap.insert("BrowsU_C", "au_right_inner_brow_raiser");
    blendshapeMap.insert("BrowsD_R", "au_right_brow_lowerer");
    
    QHash<QByteArray, int> blendshapeIndices;
    for (int i = 0;; i++) {
        QByteArray blendshape = FACESHIFT_BLENDSHAPES[i];
        if (blendshape.isEmpty()) {
            break;
        }
        if (blendshape == "EyeBlink_L") {
            leftEyeBlinkIndex = i;
        
        } else if (blendshape == "EyeBlink_R") {
            rightEyeBlinkIndex = i;
            
        } else if (blendshape == "BrowsU_C") {
            centerBrowIndex = i;
        }
        QByteArray mapping = blendshapeMap.value(blendshape);
        if (!mapping.isEmpty()) {
            blendshapeIndices.insert(mapping, i + 1);
        }
    }
    
    return blendshapeIndices;
}

static const QHash<QByteArray, int>& getBlendshapeIndices() {
    static QHash<QByteArray, int> blendshapeIndices = createBlendshapeIndices();
    return blendshapeIndices;
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
    
    if (_blendshapeIndices.isEmpty()) {
        _blendshapeIndices.resize(_data->actionUnitCount);
        int maxIndex = -1;
        for (int i = 0; i < _data->actionUnitCount; i++) {
            QByteArray name = _data->actionUnitsNames[i];
            if (name == "au_left_inner_brow_raiser") {
                _leftInnerBrowIndex = i;
            } else if (name == "au_right_inner_brow_raiser") {
                _rightInnerBrowIndex = i;
            }
            int index = getBlendshapeIndices().value(_data->actionUnitsNames[i]) - 1;
            maxIndex = qMax(maxIndex, _blendshapeIndices[i] = index);
        }
        _blendshapeCoefficients.resize(maxIndex + 1);
    }
    
    qFill(_blendshapeCoefficients.begin(), _blendshapeCoefficients.end(), 0.0f);
    for (int i = 0; i < _data->actionUnitCount; i++) {
        if (!_data->actionUnitsUsed[i]) {
            continue;
        }
        int index = _blendshapeIndices.at(i);
        if (index != -1) {
            _blendshapeCoefficients[index] = _data->actionUnits[i];
        }
    }
    _blendshapeCoefficients[leftEyeBlinkIndex] = 1.0f - _data->eyeClosure[1];
    _blendshapeCoefficients[rightEyeBlinkIndex] = 1.0f - _data->eyeClosure[0];
    _blendshapeCoefficients[centerBrowIndex] = (_data->actionUnits[_leftInnerBrowIndex] +
        _data->actionUnits[_rightInnerBrowIndex]) * 0.5f;
#endif
}

void Visage::reset() {
    _headOrigin += _headTranslation / TRANSLATION_SCALE;
}
