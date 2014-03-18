//
//  AvatarData.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <QtCore/QDataStream>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>
#include <VoxelConstants.h>

#include "AvatarData.h"

using namespace std;

QNetworkAccessManager* AvatarData::networkAccessManager = NULL;

AvatarData::AvatarData() :
    _handPosition(0,0,0),
    _bodyYaw(-90.f),
    _bodyPitch(0.0f),
    _bodyRoll(0.0f),
    _targetScale(1.0f),
    _handState(0),
    _keyState(NO_KEY_DOWN),
    _isChatCirclingEnabled(false),
    _headData(NULL),
    _handData(NULL), 
    _displayNameBoundingRect(), 
    _displayNameTargetAlpha(0.0f), 
    _displayNameAlpha(0.0f),
    _billboard()
{
    
}

AvatarData::~AvatarData() {
    delete _headData;
    delete _handData;
}

glm::vec3 AvatarData::getHandPosition() const {
    return getOrientation() * _handPosition + _position;
}

void AvatarData::setHandPosition(const glm::vec3& handPosition) {
    // store relative to position/orientation
    _handPosition = glm::inverse(getOrientation()) * (handPosition - _position);
}

QByteArray AvatarData::toByteArray() {
    // TODO: DRY this up to a shared method
    // that can pack any type given the number of bytes
    // and return the number of bytes to push the pointer
    
    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    
    QByteArray avatarDataByteArray;
    avatarDataByteArray.resize(MAX_PACKET_SIZE);
    
    unsigned char* destinationBuffer = reinterpret_cast<unsigned char*>(avatarDataByteArray.data());
    unsigned char* startPosition = destinationBuffer;
    
    memcpy(destinationBuffer, &_position, sizeof(_position));
    destinationBuffer += sizeof(_position);
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyYaw);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyPitch);
    destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _bodyRoll);

    // Body scale
    destinationBuffer += packFloatRatioToTwoByte(destinationBuffer, _targetScale);

    // Lookat Position
    memcpy(destinationBuffer, &_headData->_lookAtPosition, sizeof(_headData->_lookAtPosition));
    destinationBuffer += sizeof(_headData->_lookAtPosition);
     
    // Instantaneous audio loudness (used to drive facial animation)
    memcpy(destinationBuffer, &_headData->_audioLoudness, sizeof(float));
    destinationBuffer += sizeof(float);

    // chat message
    *destinationBuffer++ = _chatMessage.size();
    memcpy(destinationBuffer, _chatMessage.data(), _chatMessage.size() * sizeof(char));
    destinationBuffer += _chatMessage.size() * sizeof(char);
    
    // bitMask of less than byte wide items
    unsigned char bitItems = 0;

    // key state
    setSemiNibbleAt(bitItems,KEY_STATE_START_BIT,_keyState);
    // hand state
    setSemiNibbleAt(bitItems,HAND_STATE_START_BIT,_handState);
    // faceshift state
    if (_headData->_isFaceshiftConnected) { setAtBit(bitItems, IS_FACESHIFT_CONNECTED); }
    if (_isChatCirclingEnabled) {
        setAtBit(bitItems, IS_CHAT_CIRCLING_ENABLED);
    }
    *destinationBuffer++ = bitItems;

    // If it is connected, pack up the data
    if (_headData->_isFaceshiftConnected) {
        memcpy(destinationBuffer, &_headData->_leftEyeBlink, sizeof(float));
        destinationBuffer += sizeof(float);

        memcpy(destinationBuffer, &_headData->_rightEyeBlink, sizeof(float));
        destinationBuffer += sizeof(float);

        memcpy(destinationBuffer, &_headData->_averageLoudness, sizeof(float));
        destinationBuffer += sizeof(float);

        memcpy(destinationBuffer, &_headData->_browAudioLift, sizeof(float));
        destinationBuffer += sizeof(float);
        
        *destinationBuffer++ = _headData->_blendshapeCoefficients.size();
        memcpy(destinationBuffer, _headData->_blendshapeCoefficients.data(),
            _headData->_blendshapeCoefficients.size() * sizeof(float));
        destinationBuffer += _headData->_blendshapeCoefficients.size() * sizeof(float);
    }
    
    // pupil dilation
    destinationBuffer += packFloatToByte(destinationBuffer, _headData->_pupilDilation, 1.0f);

    // joint data
    *destinationBuffer++ = _jointData.size();
    unsigned char validity = 0;
    int validityBit = 0;
    foreach (const JointData& data, _jointData) {
        if (data.valid) {
            validity |= (1 << validityBit);
        }
        if (++validityBit == BITS_IN_BYTE) {
            *destinationBuffer++ = validity;
            validityBit = validity = 0;
        }
    }
    if (validityBit != 0) {
        *destinationBuffer++ = validity;
    }
    foreach (const JointData& data, _jointData) {
        if (data.valid) {
            destinationBuffer += packOrientationQuatToBytes(destinationBuffer, data.rotation);
        }
    }
        
    return avatarDataByteArray.left(destinationBuffer - startPosition);
}

// read data in packet starting at byte offset and return number of bytes parsed
int AvatarData::parseDataAtOffset(const QByteArray& packet, int offset) {

    // lazily allocate memory for HeadData in case we're not an Avatar instance
    if (!_headData) {
        _headData = new HeadData(this);
    }
    
    // lazily allocate memory for HandData in case we're not an Avatar instance
    if (!_handData) {
        _handData = new HandData(this);
    }
    
    const unsigned char* startPosition = reinterpret_cast<const unsigned char*>(packet.data());
    const unsigned char* sourceBuffer = startPosition + offset;
    
    // Body world position
    memcpy(&_position, sourceBuffer, sizeof(float) * 3);
    sourceBuffer += sizeof(float) * 3;
    
    // Body rotation (NOTE: This needs to become a quaternion to save two bytes)
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyYaw);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyPitch);
    sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_bodyRoll);
    
    // Body scale
    sourceBuffer += unpackFloatRatioFromTwoByte(sourceBuffer, _targetScale);
    
    // Lookat Position
    memcpy(&_headData->_lookAtPosition, sourceBuffer, sizeof(_headData->_lookAtPosition));
    sourceBuffer += sizeof(_headData->_lookAtPosition);
    
    // Instantaneous audio loudness (used to drive facial animation)
    memcpy(&_headData->_audioLoudness, sourceBuffer, sizeof(float));
    sourceBuffer += sizeof(float);
    
    // the rest is a chat message
    int chatMessageSize = *sourceBuffer++;
    _chatMessage = string((char*)sourceBuffer, chatMessageSize);
    sourceBuffer += chatMessageSize * sizeof(char);
    
    // voxel sending features...
    unsigned char bitItems = 0;
    bitItems = (unsigned char)*sourceBuffer++;
    
    // key state, stored as a semi-nibble in the bitItems
    _keyState = (KeyState)getSemiNibbleAt(bitItems,KEY_STATE_START_BIT);
    
    // hand state, stored as a semi-nibble in the bitItems
    _handState = getSemiNibbleAt(bitItems,HAND_STATE_START_BIT);
    
    _headData->_isFaceshiftConnected = oneAtBit(bitItems, IS_FACESHIFT_CONNECTED);
    
    _isChatCirclingEnabled = oneAtBit(bitItems, IS_CHAT_CIRCLING_ENABLED);
    
    // If it is connected, pack up the data
    if (_headData->_isFaceshiftConnected) {
        memcpy(&_headData->_leftEyeBlink, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        
        memcpy(&_headData->_rightEyeBlink, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        
        memcpy(&_headData->_averageLoudness, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        
        memcpy(&_headData->_browAudioLift, sourceBuffer, sizeof(float));
        sourceBuffer += sizeof(float);
        
        _headData->_blendshapeCoefficients.resize(*sourceBuffer++);
        memcpy(_headData->_blendshapeCoefficients.data(), sourceBuffer,
               _headData->_blendshapeCoefficients.size() * sizeof(float));
        sourceBuffer += _headData->_blendshapeCoefficients.size() * sizeof(float);
    }
    
    // pupil dilation
    sourceBuffer += unpackFloatFromByte(sourceBuffer, _headData->_pupilDilation, 1.0f);
    
    // joint data
    int jointCount = *sourceBuffer++;
    _jointData.resize(jointCount);
    unsigned char validity = 0;
    int validityBit = 0;
    for (int i = 0; i < jointCount; i++) {
        if (validityBit == 0) {
            validity = *sourceBuffer++;
        }   
        _jointData[i].valid = (bool)(validity & (1 << validityBit));
        validityBit = (validityBit + 1) % BITS_IN_BYTE; 
    }
    for (int i = 0; i < jointCount; i++) {
        JointData& data = _jointData[i];
        if (data.valid) {
            sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, data.rotation);
        }
    }
    
    return sourceBuffer - startPosition;
}

void AvatarData::setJointData(int index, const glm::quat& rotation) {
    if (index == -1) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(int, index), Q_ARG(const glm::quat&, rotation));
        return;
    }
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    JointData& data = _jointData[index];
    data.valid = true;
    data.rotation = rotation;
}

void AvatarData::clearJointData(int index) {
    if (index == -1) {
        return;
    }
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(int, index));
        return;
    }
    if (_jointData.size() <= index) {
        _jointData.resize(index + 1);
    }
    _jointData[index].valid = false;
}

bool AvatarData::isJointDataValid(int index) const {
    if (index == -1) {
        return false;
    }
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "isJointDataValid", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result), Q_ARG(int, index));
        return result;
    }
    return index < _jointData.size() && _jointData.at(index).valid;
}

glm::quat AvatarData::getJointRotation(int index) const {
    if (index == -1) {
        return glm::quat();
    }
    if (QThread::currentThread() != thread()) {
        glm::quat result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "getJointRotation", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(glm::quat, result), Q_ARG(int, index));
        return result;
    }
    return index < _jointData.size() ? _jointData.at(index).rotation : glm::quat();
}

void AvatarData::setJointData(const QString& name, const glm::quat& rotation) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setJointData", Q_ARG(const QString&, name),
            Q_ARG(const glm::quat&, rotation));
        return;
    }
    setJointData(getJointIndex(name), rotation);
}

void AvatarData::clearJointData(const QString& name) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearJointData", Q_ARG(const QString&, name));
        return;
    }
    clearJointData(getJointIndex(name));
}

bool AvatarData::isJointDataValid(const QString& name) const {
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "isJointDataValid", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result), Q_ARG(const QString&, name));
        return result;
    }
    return isJointDataValid(getJointIndex(name));
}

glm::quat AvatarData::getJointRotation(const QString& name) const {
    if (QThread::currentThread() != thread()) {
        glm::quat result;
        QMetaObject::invokeMethod(const_cast<AvatarData*>(this), "getJointRotation", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(glm::quat, result), Q_ARG(const QString&, name));
        return result;
    }
    return getJointRotation(getJointIndex(name));
}

bool AvatarData::hasIdentityChangedAfterParsing(const QByteArray &packet) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    QUuid avatarUUID;
    QUrl faceModelURL, skeletonModelURL;
    QString displayName;
    packetStream >> avatarUUID >> faceModelURL >> skeletonModelURL >> displayName;
    
    bool hasIdentityChanged = false;
    
    if (faceModelURL != _faceModelURL) {
        setFaceModelURL(faceModelURL);
        hasIdentityChanged = true;
    }
    
    if (skeletonModelURL != _skeletonModelURL) {
        setSkeletonModelURL(skeletonModelURL);
        hasIdentityChanged = true;
    }

    if (displayName != _displayName) {
        setDisplayName(displayName);
        hasIdentityChanged = true;
    }
        
    return hasIdentityChanged;
}

QByteArray AvatarData::identityByteArray() {
    QByteArray identityData;
    QDataStream identityStream(&identityData, QIODevice::Append);

    identityStream << QUuid() << _faceModelURL << _skeletonModelURL << _displayName;
    
    return identityData;
}

bool AvatarData::hasBillboardChangedAfterParsing(const QByteArray& packet) {
    QByteArray newBillboard = packet.mid(numBytesForPacketHeader(packet));
    if (newBillboard == _billboard) {
        return false;
    }
    _billboard = newBillboard;
    return true;
}

void AvatarData::setFaceModelURL(const QUrl& faceModelURL) {
    _faceModelURL = faceModelURL.isEmpty() ? DEFAULT_HEAD_MODEL_URL : faceModelURL;
    
    qDebug() << "Changing face model for avatar to" << _faceModelURL.toString();
}

void AvatarData::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    _skeletonModelURL = skeletonModelURL.isEmpty() ? DEFAULT_BODY_MODEL_URL : skeletonModelURL;
    
    qDebug() << "Changing skeleton model for avatar to" << _skeletonModelURL.toString();
}

void AvatarData::setDisplayName(const QString& displayName) {
    _displayName = displayName;

    qDebug() << "Changing display name for avatar to" << displayName;
}

void AvatarData::setBillboard(const QByteArray& billboard) {
    _billboard = billboard;
    
    qDebug() << "Changing billboard for avatar.";
}

void AvatarData::setBillboardFromURL(const QString &billboardURL) {
    _billboardURL = billboardURL;
    
    if (AvatarData::networkAccessManager) {
        qDebug() << "Changing billboard for avatar to PNG at" << qPrintable(billboardURL);
        
        QNetworkRequest billboardRequest;
        billboardRequest.setUrl(QUrl(billboardURL));
        
        QNetworkReply* networkReply = AvatarData::networkAccessManager->get(billboardRequest);
        connect(networkReply, SIGNAL(finished()), this, SLOT(setBillboardFromNetworkReply()));
        
    } else {
        qDebug() << "Billboard PNG download requested but no network access manager is available.";
    }
}

void AvatarData::setBillboardFromNetworkReply() {
    QNetworkReply* networkReply = reinterpret_cast<QNetworkReply*>(sender());
    setBillboard(networkReply->readAll());
}

void AvatarData::setClampedTargetScale(float targetScale) {
    
    targetScale =  glm::clamp(targetScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE);
    
    _targetScale = targetScale;
    qDebug() << "Changed scale to " << _targetScale;
}

void AvatarData::setOrientation(const glm::quat& orientation) {
    glm::vec3 eulerAngles = glm::degrees(safeEulerAngles(orientation));
    _bodyPitch = eulerAngles.x;
    _bodyYaw = eulerAngles.y;
    _bodyRoll = eulerAngles.z;
}

void AvatarData::sendIdentityPacket() {
    QByteArray identityPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarIdentity);
    identityPacket.append(identityByteArray());
    
    NodeList::getInstance()->broadcastToNodes(identityPacket, NodeSet() << NodeType::AvatarMixer);
}

void AvatarData::sendBillboardPacket() {
    if (!_billboard.isEmpty()) {
        QByteArray billboardPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarBillboard);
        billboardPacket.append(_billboard);
        
        NodeList::getInstance()->broadcastToNodes(billboardPacket, NodeSet() << NodeType::AvatarMixer);
    }
}
