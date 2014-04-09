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

#include "Application.h"
#include "Faceplus.h"
#include "renderer/FBXReader.h"

Faceplus::Faceplus() :
    _enabled(false),
    _active(false) {

#ifdef HAVE_FACEPLUS
    // these are ignored--any values will do
    faceplus_log_in("username", "password");
#endif
}

Faceplus::~Faceplus() {
    setEnabled(false);
}

void Faceplus::init() {
    connect(Application::getInstance()->getFaceshift(), SIGNAL(connectionStateChanged()), SLOT(updateEnabled()));
    updateEnabled();
}

#ifdef HAVE_FACEPLUS
static QMultiHash<QByteArray, QPair<int, float> > createChannelNameMap() {
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
    
    QMultiHash<QByteArray, QPair<int, float> > channelNameMap;
    for (int i = 0;; i++) {
        QByteArray blendshape = FACESHIFT_BLENDSHAPES[i];
        if (blendshape.isEmpty()) {
            break;
        }
        for (QMultiHash<QByteArray, QPair<QByteArray, float> >::const_iterator it = blendshapeMap.constFind(blendshape);
                it != blendshapeMap.constEnd() && it.key() == blendshape; it++) {
            channelNameMap.insert(it.value().first, QPair<int, float>(i, it.value().second));
        }
    }
    
    return channelNameMap;
}

static const QMultiHash<QByteArray, QPair<int, float> >& getChannelNameMap() {
    static QMultiHash<QByteArray, QPair<int, float> > channelNameMap = createChannelNameMap();
    return channelNameMap;
}
#endif

void Faceplus::update() {
#ifdef HAVE_FACEPLUS
    if (!_active) {
        return;
    }
    if (!(_active = faceplus_current_output_vector(_outputVector.data()))) {
        return;
    }
    qFill(_blendshapeCoefficients.begin(), _blendshapeCoefficients.end(), 0.0f);
    for (int i = 0; i < _outputVector.size(); i++) {
        for (QMultiHash<int, QPair<int, float> >::const_iterator it = _channelIndexMap.constFind(i);
                it != _channelIndexMap.constEnd() && it.key() == i; it++) {
            _blendshapeCoefficients[it.value().first] += _outputVector.at(i) * it.value().second;
        }
    }
#endif
}

void Faceplus::reset() {
}

void Faceplus::updateEnabled() {
    setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Faceplus) &&
        !(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift) &&
            Application::getInstance()->getFaceshift()->isConnectedOrConnecting()));
}

void Faceplus::setEnabled(bool enabled) {
#ifdef HAVE_FACEPLUS
    if (_enabled == enabled) {
        return;
    }
    if ((_enabled = enabled)) {
        if (faceplus_init("VGA")) {
            qDebug() << "Faceplus initialized.";
            _active = true;
            
            int channelCount = faceplus_output_channels_count();
            _outputVector.resize(channelCount);
            
            int maxIndex = -1;
            _channelIndexMap.clear();
            for (int i = 0; i < channelCount; i++) {
                QByteArray channelName = faceplus_output_channel_name(i);
                
                qDebug() << channelName;
                
                for (QMultiHash<QByteArray, QPair<int, float> >::const_iterator it = getChannelNameMap().constFind(name);
                        it != getChannelNameMap().constEnd() && it.key() == name; it++) {
                    _channelIndexMap.insert(i, it.value());
                    maxIndex = qMax(maxIndex, it.value().first);
                }
            }
            _blendshapeCoefficients.resize(maxIndex + 1);
        }
    } else if (faceplus_teardown()) {
        qDebug() << "Faceplus torn down.";
        _active = false;
    }
#endif
}
