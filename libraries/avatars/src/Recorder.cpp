//
//  Recorder.cpp
//
//
//  Created by Clement on 8/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <GLMHelpers.h>
#include <NodeList.h>
#include <StreamUtils.h>

#include "AvatarData.h"
#include "Recorder.h"

Recorder::Recorder(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar)
{
    _timer.invalidate();
}

bool Recorder::isRecording() const {
    return _timer.isValid();
}

qint64 Recorder::elapsed() const {
    if (isRecording()) {
        return _timer.elapsed();
    } else {
        return 0;
    }
}

void Recorder::startRecording() {
    qDebug() << "Recorder::startRecording()";
    _recording->clear();
    
    RecordingContext& context = _recording->getContext();
    context.globalTimestamp = usecTimestampNow();
    context.domain = NodeList::getInstance()->getDomainHandler().getHostname();
    context.position = _avatar->getPosition();
    context.orientation = _avatar->getOrientation();
    context.scale = _avatar->getTargetScale();
    context.headModel = _avatar->getFaceModelURL().toString();
    context.skeletonModel = _avatar->getSkeletonModelURL().toString();
    context.displayName = _avatar->getDisplayName();
    context.attachments = _avatar->getAttachmentData();
    
    context.orientationInv = glm::inverse(context.orientation);
    
    bool wantDebug = false;
    if (wantDebug) {
        qDebug() << "Recorder::startRecording(): Recording Context";
        qDebug() << "Global timestamp:" << context.globalTimestamp;
        qDebug() << "Domain:" << context.domain;
        qDebug() << "Position:" << context.position;
        qDebug() << "Orientation:" << context.orientation;
        qDebug() << "Scale:" << context.scale;
        qDebug() << "Head URL:" << context.headModel;
        qDebug() << "Skeleton URL:" << context.skeletonModel;
        qDebug() << "Display Name:" << context.displayName;
        qDebug() << "Num Attachments:" << context.attachments.size();
        
        for (int i = 0; i < context.attachments.size(); ++i) {
            qDebug() << "Model URL:" << context.attachments[i].modelURL;
            qDebug() << "Joint Name:" << context.attachments[i].jointName;
            qDebug() << "Translation:" << context.attachments[i].translation;
            qDebug() << "Rotation:" << context.attachments[i].rotation;
            qDebug() << "Scale:" << context.attachments[i].scale;
        }
    }
    
    _timer.start();
    record();
}

void Recorder::stopRecording() {
    qDebug() << "Recorder::stopRecording()";
    _timer.invalidate();
    
    qDebug().nospace() << "Recorded " << _recording->getFrameNumber() << " during " << _recording->getLength() << " msec (" << _recording->getFrameNumber() / (_recording->getLength() / 1000.0f) << " fps)";
}

void Recorder::saveToFile(QString& file) {
    if (_recording->isEmpty()) {
        qDebug() << "Cannot save recording to file, recording is empty.";
    }
    
    writeRecordingToFile(_recording, file);
}

void Recorder::record() {
    if (isRecording()) {
        const RecordingContext& context = _recording->getContext();
        RecordingFrame frame;
        frame.setBlendshapeCoefficients(_avatar->getHeadData()->getBlendshapeCoefficients());
        frame.setJointRotations(_avatar->getJointRotations());
        frame.setTranslation(context.orientationInv * (_avatar->getPosition() - context.position));
        frame.setRotation(context.orientationInv * _avatar->getOrientation());
        frame.setScale(_avatar->getTargetScale() / context.scale);
        
        
        const HeadData* head = _avatar->getHeadData();
        if (head) {
            glm::vec3 rotationDegrees = glm::vec3(head->getFinalPitch(),
                                                  head->getFinalYaw(),
                                                  head->getFinalRoll());
            frame.setHeadRotation(glm::quat(glm::radians(rotationDegrees)));
            frame.setLeanForward(head->getLeanForward());
            frame.setLeanSideways(head->getLeanSideways());
            glm::vec3 relativeLookAt = context.orientationInv *
                                       (head->getLookAtPosition() - context.position);
            frame.setLookAtPosition(relativeLookAt);
        }
        
        bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "Recording frame #" << _recording->getFrameNumber();
            qDebug() << "Blendshapes:" << frame.getBlendshapeCoefficients().size();
            qDebug() << "JointRotations:" << frame.getJointRotations().size();
            qDebug() << "Translation:" << frame.getTranslation();
            qDebug() << "Rotation:" << frame.getRotation();
            qDebug() << "Scale:" << frame.getScale();
            qDebug() << "Head rotation:" << frame.getHeadRotation();
            qDebug() << "Lean Forward:" << frame.getLeanForward();
            qDebug() << "Lean Sideways:" << frame.getLeanSideways();
            qDebug() << "LookAtPosition:" << frame.getLookAtPosition();
        }
        
        _recording->addFrame(_timer.elapsed(), frame);
    }
}

void Recorder::record(char* samples, int size) {
    QByteArray byteArray(samples, size);
    _recording->addAudioPacket(byteArray);
}
