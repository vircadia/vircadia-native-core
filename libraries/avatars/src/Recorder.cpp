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
#include "AvatarLogging.h"
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
    qCDebug(avatars) << "Recorder::startRecording()";
    _recording->clear();
    
    RecordingContext& context = _recording->getContext();
    context.globalTimestamp = usecTimestampNow();
    context.domain = DependencyManager::get<NodeList>()->getDomainHandler().getHostname();
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
        qCDebug(avatars) << "Recorder::startRecording(): Recording Context";
        qCDebug(avatars) << "Global timestamp:" << context.globalTimestamp;
        qCDebug(avatars) << "Domain:" << context.domain;
        qCDebug(avatars) << "Position:" << context.position;
        qCDebug(avatars) << "Orientation:" << context.orientation;
        qCDebug(avatars) << "Scale:" << context.scale;
        qCDebug(avatars) << "Head URL:" << context.headModel;
        qCDebug(avatars) << "Skeleton URL:" << context.skeletonModel;
        qCDebug(avatars) << "Display Name:" << context.displayName;
        qCDebug(avatars) << "Num Attachments:" << context.attachments.size();
        
        for (int i = 0; i < context.attachments.size(); ++i) {
            qCDebug(avatars) << "Model URL:" << context.attachments[i].modelURL;
            qCDebug(avatars) << "Joint Name:" << context.attachments[i].jointName;
            qCDebug(avatars) << "Translation:" << context.attachments[i].translation;
            qCDebug(avatars) << "Rotation:" << context.attachments[i].rotation;
            qCDebug(avatars) << "Scale:" << context.attachments[i].scale;
        }
    }
    
    _timer.start();
    record();
}

void Recorder::stopRecording() {
    qCDebug(avatars) << "Recorder::stopRecording()";
    _timer.invalidate();
    
    qCDebug(avatars).nospace() << "Recorded " << _recording->getFrameNumber() << " during " << _recording->getLength() << " msec (" << _recording->getFrameNumber() / (_recording->getLength() / 1000.0f) << " fps)";
}

void Recorder::saveToFile(const QString& file) {
    if (_recording->isEmpty()) {
        qCDebug(avatars) << "Cannot save recording to file, recording is empty.";
    }
    
    writeRecordingToFile(_recording, file);
}

void Recorder::record() {
    if (isRecording()) {
        const RecordingContext& context = _recording->getContext();
        RecordingFrame frame;
        frame.setBlendshapeCoefficients(_avatar->getHeadData()->getBlendshapeCoefficients());

        // Capture the full skeleton joint data
        auto& jointData = _avatar->getRawJointData();
        frame.setJointArray(jointData);

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
            qCDebug(avatars) << "Recording frame #" << _recording->getFrameNumber();
            qCDebug(avatars) << "Blendshapes:" << frame.getBlendshapeCoefficients().size();
            qCDebug(avatars) << "JointArray:" << frame.getJointArray().size();
            qCDebug(avatars) << "Translation:" << frame.getTranslation();
            qCDebug(avatars) << "Rotation:" << frame.getRotation();
            qCDebug(avatars) << "Scale:" << frame.getScale();
            qCDebug(avatars) << "Head rotation:" << frame.getHeadRotation();
            qCDebug(avatars) << "Lean Forward:" << frame.getLeanForward();
            qCDebug(avatars) << "Lean Sideways:" << frame.getLeanSideways();
            qCDebug(avatars) << "LookAtPosition:" << frame.getLookAtPosition();
        }
        
        _recording->addFrame(_timer.elapsed(), frame);
    }
}

void Recorder::recordAudio(const QByteArray& audioByteArray) {
    _recording->addAudioPacket(audioByteArray);
}
