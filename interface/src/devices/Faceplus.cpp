//
//  Faceplus.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 4/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QThread>

#ifdef HAVE_FACEPLUS
#include <faceplus.h>
#endif

#include "Application.h"
#include "Faceplus.h"
#include "renderer/FBXReader.h"

static int floatVectorMetaTypeId = qRegisterMetaType<QVector<float> >();

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

void Faceplus::reset() {
	if (_enabled) {
		QMetaObject::invokeMethod(_reader, "reset");
	}
}

void Faceplus::setState(const glm::vec3& headTranslation, const glm::quat& headRotation,
		float estimatedEyePitch, float estimatedEyeYaw, const QVector<float>& blendshapeCoefficients) {
    _headTranslation = headTranslation;
	_headRotation = headRotation;
    _estimatedEyePitch = estimatedEyePitch;
    _estimatedEyeYaw = estimatedEyeYaw;
    _blendshapeCoefficients = blendshapeCoefficients;
    _active = true;
}

void Faceplus::updateEnabled() {
    setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Faceplus) &&
        !(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift) &&
            Application::getInstance()->getFaceshift()->isConnectedOrConnecting()));
}

void Faceplus::setEnabled(bool enabled) {
    if (_enabled == enabled) {
        return;
    }
    if ((_enabled = enabled)) {
        _reader = new FaceplusReader();
        QThread* readerThread = new QThread(this);
        _reader->moveToThread(readerThread);
        readerThread->start();
        QMetaObject::invokeMethod(_reader, "init");
        
    } else {
        QThread* readerThread = _reader->thread();
        QMetaObject::invokeMethod(_reader, "shutdown");
        readerThread->wait();
        delete readerThread;
        _active = false;
    }
}

#ifdef HAVE_FACEPLUS
static QMultiHash<QByteArray, QPair<int, float> > createChannelNameMap() {
    QMultiHash<QByteArray, QPair<QByteArray, float> > blendshapeMap;
    blendshapeMap.insert("EyeBlink_L", QPair<QByteArray, float>("Mix::Blink_Left", 1.0f));
    blendshapeMap.insert("EyeBlink_R", QPair<QByteArray, float>("Mix::Blink_Right", 1.0f));
    blendshapeMap.insert("BrowsD_L", QPair<QByteArray, float>("Mix::BrowsDown_Left", 1.0f));
    blendshapeMap.insert("BrowsD_R", QPair<QByteArray, float>("Mix::BrowsDown_Right", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::BrowsIn_Left", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::BrowsIn_Right", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::BrowsOuterLower_Left", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::BrowsOuterLower_Right", 1.0f));
    blendshapeMap.insert("BrowsU_L", QPair<QByteArray, float>("Mix::BrowsUp_Left", 10.0f));
    blendshapeMap.insert("BrowsU_R", QPair<QByteArray, float>("Mix::BrowsUp_Right", 10.0f));
    blendshapeMap.insert("EyeOpen_L", QPair<QByteArray, float>("Mix::EyesWide_Left", 1.0f));
    blendshapeMap.insert("EyeOpen_R", QPair<QByteArray, float>("Mix::EyesWide_Right", 1.0f));
    blendshapeMap.insert("MouthFrown_L", QPair<QByteArray, float>("Mix::Frown_Left", 1.0f));
    blendshapeMap.insert("MouthFrown_R", QPair<QByteArray, float>("Mix::Frown_Right", 1.0f));
    blendshapeMap.insert("JawLeft", QPair<QByteArray, float>("Mix::Jaw_RotateY_Left", 1.0f));
    blendshapeMap.insert("JawRight", QPair<QByteArray, float>("Mix::Jaw_RotateY_Right", 1.0f));
    blendshapeMap.insert("LipsLowerDown", QPair<QByteArray, float>("Mix::LowerLipDown_Left", 0.5f));
    blendshapeMap.insert("LipsLowerDown", QPair<QByteArray, float>("Mix::LowerLipDown_Right", 0.5f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::LowerLipIn", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::LowerLipOut", 1.0f));
    blendshapeMap.insert("MouthLeft", QPair<QByteArray, float>("Mix::Midmouth_Left", 1.0f));
    blendshapeMap.insert("MouthRight", QPair<QByteArray, float>("Mix::Midmouth_Right", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::MouthDown", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::MouthNarrow_Left", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::MouthNarrow_Right", 1.0f));
    blendshapeMap.insert("JawOpen", QPair<QByteArray, float>("Mix::MouthOpen", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::MouthUp", 1.0f));
    blendshapeMap.insert("LipsPucker", QPair<QByteArray, float>("Mix::MouthWhistle_NarrowAdjust_Left", 0.5f));
    blendshapeMap.insert("LipsPucker", QPair<QByteArray, float>("Mix::MouthWhistle_NarrowAdjust_Right", 0.5f));
    blendshapeMap.insert("Sneer", QPair<QByteArray, float>("Mix::NoseScrunch_Left", 0.5f));
    blendshapeMap.insert("Sneer", QPair<QByteArray, float>("Mix::NoseScrunch_Right", 0.5f));
    blendshapeMap.insert("MouthSmile_L", QPair<QByteArray, float>("Mix::Smile_Left", 1.0f));
    blendshapeMap.insert("MouthSmile_R", QPair<QByteArray, float>("Mix::Smile_Right", 1.0f));
    blendshapeMap.insert("EyeSquint_L", QPair<QByteArray, float>("Mix::Squint_Left", 1.0f));
    blendshapeMap.insert("EyeSquint_R", QPair<QByteArray, float>("Mix::Squint_Right", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::UpperLipIn", 1.0f));
    blendshapeMap.insert("...", QPair<QByteArray, float>("Mix::UpperLipOut", 1.0f));
    blendshapeMap.insert("LipsUpperUp", QPair<QByteArray, float>("Mix::UpperLipUp_Left", 0.5f));
    blendshapeMap.insert("LipsUpperUp", QPair<QByteArray, float>("Mix::UpperLipUp_Right", 0.5f));
    
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

FaceplusReader::~FaceplusReader() {
#ifdef HAVE_FACEPLUS
    if (faceplus_teardown()) {
        qDebug() << "Faceplus torn down.";
    }
#endif
}

void FaceplusReader::init() {
#ifdef HAVE_FACEPLUS
    if (!faceplus_init("hHD")) {
        qDebug() << "Failed to initialized Faceplus.";
        return;
    }
    qDebug() << "Faceplus initialized.";
    
    int channelCount = faceplus_output_channels_count();
    _outputVector.resize(channelCount);
    
    int maxIndex = -1;
    _channelIndexMap.clear();
    for (int i = 0; i < channelCount; i++) {
        QByteArray name = faceplus_output_channel_name(i);
        if (name == "Head_Joint::Rotation_X") {
            _headRotationIndices[0] = i;
        
        } else if (name == "Head_Joint::Rotation_Y") {
            _headRotationIndices[1] = i;
        
        } else if (name == "Head_Joint::Rotation_Z") {
            _headRotationIndices[2] = i;
        
        } else if (name == "Left_Eye_Joint::Rotation_X") {
            _leftEyeRotationIndices[0] = i;
        
        } else if (name == "Left_Eye_Joint::Rotation_Y") {
            _leftEyeRotationIndices[1] = i;
        
        } else if (name == "Right_Eye_Joint::Rotation_X") {
            _rightEyeRotationIndices[0] = i;
        
        } else if (name == "Right_Eye_Joint::Rotation_Y") {
            _rightEyeRotationIndices[1] = i;
        }
        for (QMultiHash<QByteArray, QPair<int, float> >::const_iterator it = getChannelNameMap().constFind(name);
                it != getChannelNameMap().constEnd() && it.key() == name; it++) {
            _channelIndexMap.insert(i, it.value());
            maxIndex = qMax(maxIndex, it.value().first);
        }
    }
    _blendshapeCoefficients.resize(maxIndex + 1);
    _referenceInitialized = false;

    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
#endif
}

void FaceplusReader::shutdown() {
    deleteLater();
    thread()->quit();
}

void FaceplusReader::update() {
#ifdef HAVE_FACEPLUS
    float x, y, rotation, scale;
	if (!(faceplus_synchronous_track() && faceplus_current_face_location(&x, &y, &rotation, &scale) && !glm::isnan(x) &&
			faceplus_current_output_vector(_outputVector.data()))) {
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        return;
    }
	if (!_referenceInitialized) {
		_referenceX = x;
		_referenceY = y;
		_referenceScale = scale;
		_referenceInitialized = true;
	}
	const float TRANSLATION_SCALE = 10.0f;
	const float REFERENCE_DISTANCE = 10.0f;
	float depthScale = _referenceScale / scale;
	float z = REFERENCE_DISTANCE * (depthScale - 1.0f);
	glm::vec3 headTranslation((x - _referenceX) * depthScale * TRANSLATION_SCALE,
		(y - _referenceY) * depthScale * TRANSLATION_SCALE, z);
    glm::quat headRotation(glm::radians(glm::vec3(-_outputVector.at(_headRotationIndices[0]),
        _outputVector.at(_headRotationIndices[1]), -_outputVector.at(_headRotationIndices[2]))));
    float estimatedEyePitch = (_outputVector.at(_leftEyeRotationIndices[0]) +
        _outputVector.at(_rightEyeRotationIndices[0])) * -0.5f;
    float estimatedEyeYaw = (_outputVector.at(_leftEyeRotationIndices[1]) +
        _outputVector.at(_rightEyeRotationIndices[1])) * 0.5f;
    
    qFill(_blendshapeCoefficients.begin(), _blendshapeCoefficients.end(), 0.0f);
    for (int i = 0; i < _outputVector.size(); i++) {
        for (QMultiHash<int, QPair<int, float> >::const_iterator it = _channelIndexMap.constFind(i);
                it != _channelIndexMap.constEnd() && it.key() == i; it++) {
            _blendshapeCoefficients[it.value().first] += _outputVector.at(i) * it.value().second;
        }
    }

    QMetaObject::invokeMethod(Application::getInstance()->getFaceplus(), "setState", Q_ARG(const glm::vec3&, headTranslation),
		Q_ARG(const glm::quat&, headRotation), Q_ARG(float, estimatedEyePitch), Q_ARG(float, estimatedEyeYaw),
		Q_ARG(const QVector<float>&, _blendshapeCoefficients));

    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
#endif
}

void FaceplusReader::reset() {
#ifdef HAVE_FACEPLUS
	_referenceInitialized = false;
#endif
}
