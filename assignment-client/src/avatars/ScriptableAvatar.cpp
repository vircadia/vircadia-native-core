//
//  ScriptableAvatar.cpp
//  assignment-client/src/avatars
//
//  Created by Clement on 7/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptableAvatar.h"

#include <QDebug>
#include <QThread>
#include <glm/gtx/transform.hpp>

#include <shared/QtHelpers.h>
#include <AnimUtil.h>
#include <ClientTraitsHandler.h>
#include <GLMHelpers.h>
#include <ResourceRequestObserver.h>
#include <AvatarLogging.h>
#include <EntityItem.h>
#include <EntityItemProperties.h>


ScriptableAvatar::ScriptableAvatar() {
    _clientTraitsHandler.reset(new ClientTraitsHandler(this));
}

QByteArray ScriptableAvatar::toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking) {
    _globalPosition = getWorldPosition();
    return AvatarData::toByteArrayStateful(dataDetail, dropFaceTracking);
}


// hold and priority unused but kept so that client side JS can run.
void ScriptableAvatar::startAnimation(const QString& url, float fps, float priority,
                              bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startAnimation", Q_ARG(const QString&, url), Q_ARG(float, fps),
                                  Q_ARG(float, priority), Q_ARG(bool, loop), Q_ARG(bool, hold), Q_ARG(float, firstFrame),
                                  Q_ARG(float, lastFrame), Q_ARG(const QStringList&, maskedJoints));
        return;
    }
    _animation = DependencyManager::get<AnimationCache>()->getAnimation(url);
    _animationDetails = AnimationDetails("", QUrl(url), fps, 0, loop, hold, false, firstFrame, lastFrame, true, firstFrame, false);
    _maskedJoints = maskedJoints;
}

void ScriptableAvatar::stopAnimation() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopAnimation");
        return;
    }
    _animation.clear();
}

AnimationDetails ScriptableAvatar::getAnimationDetails() {
    if (QThread::currentThread() != thread()) {
        AnimationDetails result;
        BLOCKING_INVOKE_METHOD(this, "getAnimationDetails", 
                                  Q_RETURN_ARG(AnimationDetails, result));
        return result;
    }
    return _animationDetails;
}

int ScriptableAvatar::getJointIndex(const QString& name) const {
    // Faux joints:
    int result = AvatarData::getJointIndex(name);
    if (result != -1) {
        return result;
    }
    QReadLocker readLock(&_jointDataLock);
    return _fstJointIndices.value(name) - 1;
}

QStringList ScriptableAvatar::getJointNames() const {
    QReadLocker readLock(&_jointDataLock);
    return _fstJointNames;
    return QStringList();
}

void ScriptableAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    _bind.reset();
    _animSkeleton.reset();

    AvatarData::setSkeletonModelURL(skeletonModelURL);
    updateJointMappings();
}

static AnimPose composeAnimPose(const HFMJoint& joint, const glm::quat rotation, const glm::vec3 translation) {
    glm::mat4 translationMat = glm::translate(translation);
    glm::mat4 rotationMat = glm::mat4_cast(joint.preRotation * rotation * joint.postRotation);
    glm::mat4 finalMat = translationMat * joint.preTransform * rotationMat * joint.postTransform;
    return AnimPose(finalMat);
}

void ScriptableAvatar::update(float deltatime) {
    // Run animation
    if (_animation && _animation->isLoaded() && _animation->getFrames().size() > 0 && !_bind.isNull() && _bind->isLoaded()) {
        if (!_animSkeleton) {
            _animSkeleton = std::make_shared<AnimSkeleton>(_bind->getHFMModel());
        }
        float currentFrame = _animationDetails.currentFrame + deltatime * _animationDetails.fps;
        if (_animationDetails.loop || currentFrame < _animationDetails.lastFrame) {
            while (currentFrame >= _animationDetails.lastFrame) {
                currentFrame -= (_animationDetails.lastFrame - _animationDetails.firstFrame);
            }
            _animationDetails.currentFrame = currentFrame;

            const QVector<HFMJoint>& modelJoints = _bind->getHFMModel().joints;
            QStringList animationJointNames = _animation->getJointNames();

            const int nJoints = modelJoints.size();
            if (_jointData.size() != nJoints) {
                _jointData.resize(nJoints);
            }

            const int frameCount = _animation->getFrames().size();
            const HFMAnimationFrame& floorFrame = _animation->getFrames().at((int)glm::floor(currentFrame) % frameCount);
            const HFMAnimationFrame& ceilFrame = _animation->getFrames().at((int)glm::ceil(currentFrame) % frameCount);
            const float frameFraction = glm::fract(currentFrame);
            std::vector<AnimPose> poses = _animSkeleton->getRelativeDefaultPoses();

            const float UNIT_SCALE = 0.01f;

            for (int i = 0; i < animationJointNames.size(); i++) {
                const QString& name = animationJointNames[i];
                // As long as we need the model preRotations anyway, let's get the jointIndex from the bind skeleton rather than
                // trusting the .fst (which is sometimes not updated to match changes to .fbx).
                int mapping = _bind->getHFMModel().getJointIndex(name);
                if (mapping != -1 && !_maskedJoints.contains(name)) {

                    AnimPose floorPose = composeAnimPose(modelJoints[mapping], floorFrame.rotations[i], floorFrame.translations[i] * UNIT_SCALE);
                    AnimPose ceilPose = composeAnimPose(modelJoints[mapping], ceilFrame.rotations[i], floorFrame.translations[i] * UNIT_SCALE);
                    blend(1, &floorPose, &ceilPose, frameFraction, &poses[mapping]);
                 }
            }

            std::vector<AnimPose> absPoses = poses;
            _animSkeleton->convertRelativePosesToAbsolute(absPoses);
            for (int i = 0; i < nJoints; i++) {
                JointData& data = _jointData[i];
                AnimPose& absPose = absPoses[i];
                if (data.rotation != absPose.rot()) {
                    data.rotation = absPose.rot();
                    data.rotationIsDefaultPose = false;
                }
                AnimPose& relPose = poses[i];
                if (data.translation != relPose.trans()) {
                    data.translation = relPose.trans();
                    data.translationIsDefaultPose = false;
                }
            }

        } else {
            _animation.clear();
        }
    }

    _clientTraitsHandler->sendChangedTraitsToMixer();
}

void ScriptableAvatar::updateJointMappings() {
    {
        QWriteLocker writeLock(&_jointDataLock);
        _fstJointIndices.clear();
        _fstJointNames.clear();
        _jointData.clear();
    }

    if (_skeletonModelURL.fileName().toLower().endsWith(".fst")) {
        ////
        // TODO: Should we rely upon HTTPResourceRequest for ResourceRequestObserver instead?
        // HTTPResourceRequest::doSend() covers all of the following and
        // then some. It doesn't cover the connect() call, so we may
        // want to add a HTTPResourceRequest::doSend() method that does
        // connects.
        QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkRequest networkRequest = QNetworkRequest(_skeletonModelURL);
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
        DependencyManager::get<ResourceRequestObserver>()->update(
            _skeletonModelURL, -1, "AvatarData::updateJointMappings");
        QNetworkReply* networkReply = networkAccessManager.get(networkRequest);
        //
        ////
        connect(networkReply, &QNetworkReply::finished, this, &ScriptableAvatar::setJointMappingsFromNetworkReply);
    }
}

void ScriptableAvatar::setJointMappingsFromNetworkReply() {
    QNetworkReply* networkReply = static_cast<QNetworkReply*>(sender());
    // before we process this update, make sure that the skeleton model URL hasn't changed
    // since we made the FST request
    if (networkReply->url() != _skeletonModelURL) {
        qCDebug(avatars) << "Refusing to set joint mappings for FST URL that does not match the current URL";
        networkReply->deleteLater();
        return;
    }
    {
        QWriteLocker writeLock(&_jointDataLock);
        QByteArray line;
        while (!(line = networkReply->readLine()).isEmpty()) {
            line = line.trimmed();
            if (line.startsWith("filename")) {
                int filenameIndex = line.indexOf('=') + 1;
                if (filenameIndex > 0) {
                    _skeletonFBXURL = _skeletonModelURL.resolved(QString(line.mid(filenameIndex).trimmed()));
                }
            }
            if (!line.startsWith("jointIndex")) {
                continue;
            }
            int jointNameIndex = line.indexOf('=') + 1;
            if (jointNameIndex == 0) {
                continue;
            }
            int secondSeparatorIndex = line.indexOf('=', jointNameIndex);
            if (secondSeparatorIndex == -1) {
                continue;
            }
            QString jointName = line.mid(jointNameIndex, secondSeparatorIndex - jointNameIndex).trimmed();
            bool ok;
            int jointIndex = line.mid(secondSeparatorIndex + 1).trimmed().toInt(&ok);
            if (ok) {
                while (_fstJointNames.size() < jointIndex + 1) {
                    _fstJointNames.append(QString());
                }
                _fstJointNames[jointIndex] = jointName;
            }
        }
        for (int i = 0; i < _fstJointNames.size(); i++) {
            _fstJointIndices.insert(_fstJointNames.at(i), i + 1);
        }
    }
    networkReply->deleteLater();
}

void ScriptableAvatar::setHasProceduralBlinkFaceMovement(bool hasProceduralBlinkFaceMovement) {
    _headData->setHasProceduralBlinkFaceMovement(hasProceduralBlinkFaceMovement);
}

void ScriptableAvatar::setHasProceduralEyeFaceMovement(bool hasProceduralEyeFaceMovement) {
    _headData->setHasProceduralEyeFaceMovement(hasProceduralEyeFaceMovement);
}

void ScriptableAvatar::setHasAudioEnabledFaceMovement(bool hasAudioEnabledFaceMovement) {
    _headData->setHasAudioEnabledFaceMovement(hasAudioEnabledFaceMovement);
}

void ScriptableAvatar::updateAvatarEntity(const QUuid& id, const QScriptValue& data) {
    if (data.isNull()) {
        // interpret this as a DELETE
        std::map<QUuid, EntityItemPointer>::iterator itr = _entities.find(id);
        if (itr != _entities.end()) {
            _entities.erase(itr);
            clearAvatarEntity(id);
        }
    } else {
        EntityItemPointer entity;
        EntityItemProperties properties;
        bool honorReadOnly = true;
        properties.copyFromScriptValue(data, honorReadOnly);

        std::map<QUuid, EntityItemPointer>::iterator itr = _entities.find(id);
        if (itr == _entities.end()) {
            // this is an ADD
            entity = EntityTypes::constructEntityItem(id, properties);
            if (entity) {
                OctreePacketData packetData(false, AvatarTraits::MAXIMUM_TRAIT_SIZE);
                EncodeBitstreamParams params;
                EntityTreeElementExtraEncodeDataPointer extra { nullptr };
                OctreeElement::AppendState appendState = entity->appendEntityData(&packetData, params, extra);

                if (appendState == OctreeElement::COMPLETED) {
                    _entities[id] = entity;
                    QByteArray tempArray((const char*)packetData.getUncompressedData(), packetData.getUncompressedSize());
                    storeAvatarEntityDataPayload(id, tempArray);
                }
            }
        } else {
            // this is an UPDATE
            entity = itr->second;
            bool somethingChanged = entity->setProperties(properties);
            if (somethingChanged) {
                OctreePacketData packetData(false, AvatarTraits::MAXIMUM_TRAIT_SIZE);
                EncodeBitstreamParams params;
                EntityTreeElementExtraEncodeDataPointer extra { nullptr };
                OctreeElement::AppendState appendState = entity->appendEntityData(&packetData, params, extra);

                if (appendState == OctreeElement::COMPLETED) {
                    QByteArray tempArray((const char*)packetData.getUncompressedData(), packetData.getUncompressedSize());
                    storeAvatarEntityDataPayload(id, tempArray);
                }
            }
        }
    }
}
