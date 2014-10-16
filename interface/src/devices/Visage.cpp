//
//  Visage.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QHash>

#include <PerfStat.h>
#include <SharedUtil.h>

#include <FBXReader.h>

#include "Application.h"
#include "Visage.h"

// this has to go after our normal includes, because its definition of HANDLE conflicts with Qt's
#ifdef HAVE_VISAGE
#include <VisageTracker2.h>
#endif

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
    _enabled(false),
    _active(false),
    _headOrigin(DEFAULT_HEAD_ORIGIN) {
    
#ifdef HAVE_VISAGE
#ifdef WIN32
    QByteArray licensePath = Application::resourcesPath().toLatin1() + "visage";
#else
    QByteArray licensePath = Application::resourcesPath().toLatin1() + "visage/license.vlc";
#endif
    initializeLicenseManager(licensePath.data());
    _tracker = new VisageTracker2(Application::resourcesPath().toLatin1() + "visage/tracker.cfg");
    _data = new FaceData();
#endif
}

Visage::~Visage() {
#ifdef HAVE_VISAGE
    _tracker->stop();
    // deleting the tracker crashes windows; disable for now
    //delete _tracker;
    delete _data;
#endif
}

#ifdef HAVE_VISAGE
static int leftEyeBlinkIndex = 0;
static int rightEyeBlinkIndex = 1;

static QMultiHash<QByteArray, QPair<int, float> > createActionUnitNameMap() {
    QMultiHash<QByteArray, QPair<QByteArray, float> > blendshapeMap;
    blendshapeMap.insert("JawFwd", QPair<QByteArray, float>("au_jaw_z_push", 1.0f));
    blendshapeMap.insert("JawLeft", QPair<QByteArray, float>("au_jaw_x_push", 1.0f));
    blendshapeMap.insert("JawOpen", QPair<QByteArray, float>("au_jaw_drop", 1.0f));
    blendshapeMap.insert("LipsLowerDown", QPair<QByteArray, float>("au_lower_lip_drop", 1.0f));
    blendshapeMap.insert("LipsUpperOpen", QPair<QByteArray, float>("au_upper_lip_raiser", 1.0f));
    blendshapeMap.insert("LipsStretch_R", QPair<QByteArray, float>("au_lip_stretcher_left", 0.5f));
    blendshapeMap.insert("MouthSmile_L", QPair<QByteArray, float>("au_lip_corner_depressor", -1.0f));
    blendshapeMap.insert("MouthSmile_R", QPair<QByteArray, float>("au_lip_corner_depressor", -1.0f));
    blendshapeMap.insert("BrowsU_R", QPair<QByteArray, float>("au_left_outer_brow_raiser", 1.0f));
    blendshapeMap.insert("BrowsU_C", QPair<QByteArray, float>("au_left_inner_brow_raiser", 1.0f));
    blendshapeMap.insert("BrowsD_R", QPair<QByteArray, float>("au_left_brow_lowerer", 1.0f));
    blendshapeMap.insert("EyeBlink_L", QPair<QByteArray, float>("au_leye_closed", 1.0f));
    blendshapeMap.insert("EyeBlink_R", QPair<QByteArray, float>("au_reye_closed", 1.0f));
    blendshapeMap.insert("EyeOpen_L", QPair<QByteArray, float>("au_upper_lid_raiser", 1.0f));
    blendshapeMap.insert("EyeOpen_R", QPair<QByteArray, float>("au_upper_lid_raiser", 1.0f));
    blendshapeMap.insert("LipLowerOpen", QPair<QByteArray, float>("au_lower_lip_x_push", 1.0f));
    blendshapeMap.insert("LipsStretch_L", QPair<QByteArray, float>("au_lip_stretcher_right", 0.5f));
    blendshapeMap.insert("BrowsU_L", QPair<QByteArray, float>("au_right_outer_brow_raiser", 1.0f));
    blendshapeMap.insert("BrowsU_C", QPair<QByteArray, float>("au_right_inner_brow_raiser", 1.0f));
    blendshapeMap.insert("BrowsD_L", QPair<QByteArray, float>("au_right_brow_lowerer", 1.0f));
    
    QMultiHash<QByteArray, QPair<int, float> > actionUnitNameMap;
    for (int i = 0;; i++) {
        QByteArray blendshape = FACESHIFT_BLENDSHAPES[i];
        if (blendshape.isEmpty()) {
            break;
        }
        if (blendshape == "EyeBlink_L") {
            leftEyeBlinkIndex = i;
        
        } else if (blendshape == "EyeBlink_R") {
            rightEyeBlinkIndex = i;   
        }
        for (QMultiHash<QByteArray, QPair<QByteArray, float> >::const_iterator it = blendshapeMap.constFind(blendshape);
                it != blendshapeMap.constEnd() && it.key() == blendshape; it++) {
            actionUnitNameMap.insert(it.value().first, QPair<int, float>(i, it.value().second));
        }
    }
    
    return actionUnitNameMap;
}

static const QMultiHash<QByteArray, QPair<int, float> >& getActionUnitNameMap() {
    static QMultiHash<QByteArray, QPair<int, float> > actionUnitNameMap = createActionUnitNameMap();
    return actionUnitNameMap;
}
#endif

const float TRANSLATION_SCALE = 20.0f;

void Visage::init() {
    connect(Application::getInstance()->getFaceshift(), SIGNAL(connectionStateChanged()), SLOT(updateEnabled()));
    updateEnabled();
}

void Visage::update() {
#ifdef HAVE_VISAGE
    _active = (_tracker->getTrackingData(_data) == TRACK_STAT_OK);
    if (!_active) {
        return;
    }
    PerformanceTimer perfTimer("visage");
    _headRotation = glm::quat(glm::vec3(-_data->faceRotation[0], -_data->faceRotation[1], _data->faceRotation[2]));    
    _headTranslation = (glm::vec3(_data->faceTranslation[0], _data->faceTranslation[1], _data->faceTranslation[2]) -
        _headOrigin) * TRANSLATION_SCALE;
    _estimatedEyePitch = glm::degrees(-_data->gazeDirection[1]);
    _estimatedEyeYaw = glm::degrees(-_data->gazeDirection[0]);
    
    if (_actionUnitIndexMap.isEmpty()) {
        int maxIndex = -1;
        for (int i = 0; i < _data->actionUnitCount; i++) {
            QByteArray name = _data->actionUnitsNames[i];
            for (QMultiHash<QByteArray, QPair<int, float> >::const_iterator it = getActionUnitNameMap().constFind(name);
                    it != getActionUnitNameMap().constEnd() && it.key() == name; it++) {
                _actionUnitIndexMap.insert(i, it.value());
                maxIndex = qMax(maxIndex, it.value().first);
            }
        }
        _blendshapeCoefficients.resize(maxIndex + 1);
    }
    
    qFill(_blendshapeCoefficients.begin(), _blendshapeCoefficients.end(), 0.0f);
    for (int i = 0; i < _data->actionUnitCount; i++) {
        if (!_data->actionUnitsUsed[i]) {
            continue;
        }
        for (QMultiHash<int, QPair<int, float> >::const_iterator it = _actionUnitIndexMap.constFind(i);
                it != _actionUnitIndexMap.constEnd() && it.key() == i; it++) {
            _blendshapeCoefficients[it.value().first] += _data->actionUnits[i] * it.value().second;
        }
    }
    _blendshapeCoefficients[leftEyeBlinkIndex] = 1.0f - _data->eyeClosure[1];
    _blendshapeCoefficients[rightEyeBlinkIndex] = 1.0f - _data->eyeClosure[0];
#endif
}

void Visage::reset() {
    _headOrigin += _headTranslation / TRANSLATION_SCALE;
}

void Visage::updateEnabled() {
    setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Visage) &&
        !(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift) &&
            Application::getInstance()->getFaceshift()->isConnectedOrConnecting()));
}

void Visage::setEnabled(bool enabled) {
#ifdef HAVE_VISAGE
    if (_enabled == enabled) {
        return;
    }
    if ((_enabled = enabled)) {
        _tracker->trackFromCam();
    } else {
        _tracker->stop();
    }
#endif
}
