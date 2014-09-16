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
#include <NetworkAccessManager.h>

#include <QEventLoop>
#include <QFile>
#include <QMetaObject>
#include <QObject>

#include "AvatarData.h"
#include <Nodelist.h>
#include "Recorder.h"


#include <GLMHelpers.h>


// TODO: remove operators
void operator<<(QDebug& stream, glm::vec3 vector) {
    stream << vector.x << vector.y << vector.z;
}
void operator<<(QDebug& stream, glm::quat quat) {
    stream << quat.x << quat.y << quat.z << quat.w;
}

void RecordingFrame::setBlendshapeCoefficients(QVector<float> blendshapeCoefficients) {
    _blendshapeCoefficients = blendshapeCoefficients;
}

Recording::Recording() : _audio(NULL) {
}

Recording::~Recording() {
    delete _audio;
}

int Recording::getLength() const {
    if (_timestamps.isEmpty()) {
        return 0;
    }
    return _timestamps.last();
}

qint32 Recording::getFrameTimestamp(int i) const {
    if (i >= _timestamps.size()) {
        return getLength();
    }
    return _timestamps[i];
}

const RecordingFrame& Recording::getFrame(int i) const {
    assert(i < _timestamps.size());
    return _frames[i];
}

void Recording::addFrame(int timestamp, RecordingFrame &frame) {
    _timestamps << timestamp;
    _frames << frame;
}

void Recording::addAudioPacket(QByteArray byteArray) {
    if (!_audio) {
        _audio = new Sound(byteArray);
    }
    _audio->append(byteArray);
}

void Recording::clear() {
    _timestamps.clear();
    _frames.clear();
    delete _audio;
    _audio = NULL;
}

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

void Recorder::saveToFile(QString file) {
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


Player::Player(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar),
    _audioThread(NULL),
    _playFromCurrentPosition(true),
    _loop(false)
{
    _timer.invalidate();
    _options.setLoop(false);
    _options.setVolume(1.0f);
}

bool Player::isPlaying() const {
    return _timer.isValid();
}

qint64 Player::elapsed() const {
    if (isPlaying()) {
        return _timer.elapsed();
    } else {
        return 0;
    }
}

void Player::startPlaying() {
    if (_recording && _recording->getFrameNumber() > 0) {
        // Setup audio thread
        _audioThread = new QThread();
        _options.setPosition(_avatar->getPosition());
        _options.setOrientation(_avatar->getOrientation());
        _injector.reset(new AudioInjector(_recording->getAudio(), _options), &QObject::deleteLater);
        _injector->moveToThread(_audioThread);
        _audioThread->start();
        QMetaObject::invokeMethod(_injector.data(), "injectAudio", Qt::QueuedConnection);
        
        // Fake faceshift connection
        _avatar->setForceFaceshiftConnected(true);
        
        _currentContext.domain = NodeList::getInstance()->getDomainHandler().getHostname();
        _currentContext.position = _avatar->getPosition();
        _currentContext.orientation = _avatar->getOrientation();
        _currentContext.scale = _avatar->getTargetScale();
        _currentContext.headModel = _avatar->getFaceModelURL().toString();
        _currentContext.skeletonModel = _avatar->getSkeletonModelURL().toString();
        _currentContext.displayName = _avatar->getDisplayName();
        _currentContext.attachments = _avatar->getAttachmentData();
        
        _currentContext.orientationInv = glm::inverse(_currentContext.orientation);
        
        bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "Player::startPlaying(): Recording Context";
            qDebug() << "Domain:" << _currentContext.domain;
            qDebug() << "Position:" << _currentContext.position;
            qDebug() << "Orientation:" << _currentContext.orientation;
            qDebug() << "Scale:" << _currentContext.scale;
            qDebug() << "Head URL:" << _currentContext.headModel;
            qDebug() << "Skeleton URL:" << _currentContext.skeletonModel;
            qDebug() << "Display Name:" << _currentContext.displayName;
            qDebug() << "Num Attachments:" << _currentContext.attachments.size();
            
            for (int i = 0; i < _currentContext.attachments.size(); ++i) {
                qDebug() << "Model URL:" << _currentContext.attachments[i].modelURL;
                qDebug() << "Joint Name:" << _currentContext.attachments[i].jointName;
                qDebug() << "Translation:" << _currentContext.attachments[i].translation;
                qDebug() << "Rotation:" << _currentContext.attachments[i].rotation;
                qDebug() << "Scale:" << _currentContext.attachments[i].scale;
            }
        }
        
        qDebug() << "Recorder::startPlaying()";
        _currentFrame = 0;
        _timer.start();
    }
}

void Player::stopPlaying() {
    if (!isPlaying()) {
        return;
    }
    
    _timer.invalidate();
    
    _avatar->clearJointsData();
    
    // Cleanup audio thread
    _injector->stop();
    QObject::connect(_injector.data(), &AudioInjector::finished,
                     _injector.data(), &AudioInjector::deleteLater);
    QObject::connect(_injector.data(), &AudioInjector::destroyed,
                     _audioThread, &QThread::quit);
    QObject::connect(_audioThread, &QThread::finished,
                     _audioThread, &QThread::deleteLater);
    _injector.clear();
    _audioThread = NULL;
    
    // Turn off fake faceshift connection
    _avatar->setForceFaceshiftConnected(false);
    
    qDebug() << "Recorder::stopPlaying()";
}

void Player::loadFromFile(QString file) {
    if (_recording) {
        _recording->clear();
    } else {
        _recording = RecordingPointer(new Recording());
    }
    readRecordingFromFile(_recording, file);
}

void Player::loadRecording(RecordingPointer recording) {
    _recording = recording;
}

void Player::play() {
    computeCurrentFrame();
    if (_currentFrame < 0 || (_currentFrame >= _recording->getFrameNumber() - 1)) {
        // If it's the end of the recording, stop playing
        stopPlaying();
        
        if (_loop) {
            startPlaying();
        }
        return;
    }
    
    RecordingContext& context = _recording->getContext();
    if (_playFromCurrentPosition) {
        context = _currentContext;
    }
    const RecordingFrame& currentFrame = _recording->getFrame(_currentFrame);
    
    _avatar->setPosition(context.position + context.orientation * currentFrame.getTranslation());
    _avatar->setOrientation(context.orientation * currentFrame.getRotation());
    _avatar->setTargetScale(context.scale * currentFrame.getScale());
    _avatar->setJointRotations(currentFrame.getJointRotations());
    
    HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
    if (head) {
        head->setBlendshapeCoefficients(currentFrame.getBlendshapeCoefficients());
        head->setLeanSideways(currentFrame.getLeanSideways());
        head->setLeanForward(currentFrame.getLeanForward());
        glm::vec3 eulers = glm::degrees(safeEulerAngles(currentFrame.getHeadRotation()));
        head->setFinalPitch(eulers.x);
        head->setFinalYaw(eulers.y);
        head->setFinalRoll(eulers.z);
        head->setLookAtPosition(currentFrame.getLookAtPosition());
    } else {
        qDebug() << "WARNING: Player couldn't find head data.";
    }
    
    _options.setPosition(_avatar->getPosition());
    _options.setOrientation(_avatar->getOrientation());
    _injector->setOptions(_options);
}

void Player::setPlayFromCurrentLocation(bool playFromCurrentLocation) {
    _playFromCurrentPosition = playFromCurrentLocation;
}

void Player::setLoop(bool loop) {
    _loop = loop;
}

bool Player::computeCurrentFrame() {
    if (!isPlaying()) {
        _currentFrame = -1;
        return false;
    }
    if (_currentFrame < 0) {
        _currentFrame = 0;
    }
    
    while (_currentFrame < _recording->getFrameNumber() - 1 &&
           _recording->getFrameTimestamp(_currentFrame) < _timer.elapsed()) {
        ++_currentFrame;
    }
    
    return true;
}

void writeVec3(QDataStream& stream, glm::vec3 value) {
    unsigned char buffer[256];
    int writtenToBuffer = packFloatVec3ToSignedTwoByteFixed(buffer, value, 0);
    stream.writeRawData(reinterpret_cast<char*>(buffer), writtenToBuffer);
}

bool readVec3(QDataStream& stream, glm::vec3& value) {
    int vec3ByteSize = 3 * 2; // 3 floats * 2 bytes
    unsigned char buffer[256];
    stream.readRawData(reinterpret_cast<char*>(buffer), vec3ByteSize);
    int readFromBuffer = unpackFloatVec3FromSignedTwoByteFixed(buffer, value, 0);
    if (readFromBuffer != vec3ByteSize) {
        return false;
    }
    return true;
}

void writeQuat(QDataStream& stream, glm::quat value) {
    unsigned char buffer[256];
    int writtenToBuffer = packOrientationQuatToBytes(buffer, value);
    stream.writeRawData(reinterpret_cast<char*>(buffer), writtenToBuffer);
}

bool readQuat(QDataStream& stream, glm::quat& value) {
    int quatByteSize = 4 * 2; // 4 floats * 2 bytes
    unsigned char buffer[256];
    stream.readRawData(reinterpret_cast<char*>(buffer), quatByteSize);
    int readFromBuffer = unpackOrientationQuatFromBytes(buffer, value);
    if (readFromBuffer != quatByteSize) {
        return false;
    }
    return true;
}

void writeFloat(QDataStream& stream, float value) {
    unsigned char buffer[256];
    int writtenToBuffer = packFloatScalarToSignedTwoByteFixed(buffer, value, 0);
    stream.writeRawData(reinterpret_cast<char*>(buffer), writtenToBuffer);
}

bool readFloat(QDataStream& stream, float& value) {
    int floatByteSize = 2; // 1 floats * 2 bytes
    int16_t buffer[256];
    stream.readRawData(reinterpret_cast<char*>(buffer), floatByteSize);
    int readFromBuffer = unpackFloatScalarFromSignedTwoByteFixed(buffer, &value, 0);
    if (readFromBuffer != floatByteSize) {
        return false;
    }
    return true;
}

void writeRecordingToFile(RecordingPointer recording, QString filename) {
    if (!recording || recording->getFrameNumber() < 1) {
        qDebug() << "Can't save empty recording";
        return;
    }
    
    qDebug() << "Writing recording to " << filename << ".";
    QElapsedTimer timer;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)){
        return;
    }
    timer.start();
    
    
    QDataStream fileStream(&file);
    
    fileStream << recording->_timestamps;
    
    RecordingFrame& baseFrame = recording->_frames[0];
    int totalLength = 0;
    
    // Blendshape coefficients
    fileStream << baseFrame._blendshapeCoefficients;
    totalLength += baseFrame._blendshapeCoefficients.size();
    
    // Joint Rotations
    int jointRotationSize = baseFrame._jointRotations.size();
    fileStream << jointRotationSize;
    for (int i = 0; i < jointRotationSize; ++i) {
        fileStream << baseFrame._jointRotations[i].x << baseFrame._jointRotations[i].y << baseFrame._jointRotations[i].z << baseFrame._jointRotations[i].w;
    }
    totalLength += jointRotationSize;
    
    // Translation
    fileStream << baseFrame._translation.x << baseFrame._translation.y << baseFrame._translation.z;
    totalLength += 1;
    
    // Rotation
    fileStream << baseFrame._rotation.x << baseFrame._rotation.y << baseFrame._rotation.z << baseFrame._rotation.w;
    totalLength += 1;
    
    // Scale
    fileStream << baseFrame._scale;
    totalLength += 1;
    
    // Head Rotation
    fileStream << baseFrame._headRotation.x << baseFrame._headRotation.y << baseFrame._headRotation.z << baseFrame._headRotation.w;
    totalLength += 1;
    
    // Lean Sideways
    fileStream << baseFrame._leanSideways;
    totalLength += 1;
    
    // Lean Forward
    fileStream << baseFrame._leanForward;
    totalLength += 1;
    
    for (int i = 1; i < recording->_timestamps.size(); ++i) {
        QBitArray mask(totalLength);
        int maskIndex = 0;
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::WriteOnly);
        RecordingFrame& previousFrame = recording->_frames[i - 1];
        RecordingFrame& frame = recording->_frames[i];
        
        // Blendshape coefficients
        for (int i = 0; i < frame._blendshapeCoefficients.size(); ++i) {
            if (frame._blendshapeCoefficients[i] != previousFrame._blendshapeCoefficients[i]) {
                stream << frame._blendshapeCoefficients[i];
                mask.setBit(maskIndex);
            }
            maskIndex++;
        }
        
        // Joint Rotations
        for (int i = 0; i < frame._jointRotations.size(); ++i) {
            if (frame._jointRotations[i] != previousFrame._jointRotations[i]) {
                stream << frame._jointRotations[i].x << frame._jointRotations[i].y << frame._jointRotations[i].z << frame._jointRotations[i].w;
                mask.setBit(maskIndex);
            }
            maskIndex++;
        }
        
        // Translation
        if (frame._translation != previousFrame._translation) {
            stream << frame._translation.x << frame._translation.y << frame._translation.z;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Rotation
        if (frame._rotation != previousFrame._rotation) {
            stream << frame._rotation.x << frame._rotation.y << frame._rotation.z << frame._rotation.w;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Scale
        if (frame._scale != previousFrame._scale) {
            stream << frame._scale;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Head Rotation
        if (frame._headRotation != previousFrame._headRotation) {
            stream << frame._headRotation.x << frame._headRotation.y << frame._headRotation.z << frame._headRotation.w;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Lean Sideways
        if (frame._leanSideways != previousFrame._leanSideways) {
            stream << frame._leanSideways;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Lean Forward
        if (frame._leanForward != previousFrame._leanForward) {
            stream << frame._leanForward;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        fileStream << mask;
        fileStream << buffer;
    }
    
    fileStream << recording->_audio->getByteArray();
    
    qDebug() << "Wrote " << file.size() << " bytes in " << timer.elapsed() << " ms.";
}

RecordingPointer readRecordingFromFile(RecordingPointer recording, QString filename) {
    QElapsedTimer timer;
    timer.start();
    
    QByteArray byteArray;
    QUrl url(filename);
    if (url.scheme() == "http" || url.scheme() == "https" || url.scheme() == "ftp") {
        qDebug() << "Downloading recording at" << url;
        NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkReply* reply = networkAccessManager.get(QNetworkRequest(url));
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error while downloading recording: " << reply->error();
            reply->deleteLater();
            return recording;
        }
        byteArray = reply->readAll();
        reply->deleteLater();
    } else {
        qDebug() << "Reading recording from " << filename << ".";
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)){
            return recording;
        }
        byteArray = file.readAll();
        file.close();
    }
    
    if (!recording) {
        recording.reset(new Recording());
    }
    
    QDataStream fileStream(byteArray);
    
    fileStream >> recording->_timestamps;
    RecordingFrame baseFrame;
    
    // Blendshape coefficients
    fileStream >> baseFrame._blendshapeCoefficients;
    
    // Joint Rotations
    int jointRotationSize;
    fileStream >> jointRotationSize;
    baseFrame._jointRotations.resize(jointRotationSize);
    for (int i = 0; i < jointRotationSize; ++i) {
        fileStream >> baseFrame._jointRotations[i].x >> baseFrame._jointRotations[i].y >> baseFrame._jointRotations[i].z >> baseFrame._jointRotations[i].w;
    }
    
    fileStream >> baseFrame._translation.x >> baseFrame._translation.y >> baseFrame._translation.z;
    fileStream >> baseFrame._rotation.x >> baseFrame._rotation.y >> baseFrame._rotation.z >> baseFrame._rotation.w;
    fileStream >> baseFrame._scale;
    fileStream >> baseFrame._headRotation.x >> baseFrame._headRotation.y >> baseFrame._headRotation.z >> baseFrame._headRotation.w;
    fileStream >> baseFrame._leanSideways;
    fileStream >> baseFrame._leanForward;
    
    recording->_frames << baseFrame;
    
    for (int i = 1; i < recording->_timestamps.size(); ++i) {
        QBitArray mask;
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::ReadOnly);
        RecordingFrame frame;
        RecordingFrame& previousFrame = recording->_frames.last();
        
        fileStream >> mask;
        fileStream >> buffer;
        int maskIndex = 0;
        
        // Blendshape Coefficients
        frame._blendshapeCoefficients.resize(baseFrame._blendshapeCoefficients.size());
        for (int i = 0; i < baseFrame._blendshapeCoefficients.size(); ++i) {
            if (mask[maskIndex++]) {
                stream >> frame._blendshapeCoefficients[i];
            } else {
                frame._blendshapeCoefficients[i] = previousFrame._blendshapeCoefficients[i];
            }
        }
        
        // Joint Rotations
        frame._jointRotations.resize(baseFrame._jointRotations.size());
        for (int i = 0; i < baseFrame._jointRotations.size(); ++i) {
            if (mask[maskIndex++]) {
                stream >> frame._jointRotations[i].x >> frame._jointRotations[i].y >> frame._jointRotations[i].z >> frame._jointRotations[i].w;
            } else {
                frame._jointRotations[i] = previousFrame._jointRotations[i];
            }
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._translation.x >> frame._translation.y >> frame._translation.z;
        } else {
            frame._translation = previousFrame._translation;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._rotation.x >> frame._rotation.y >> frame._rotation.z >> frame._rotation.w;
        } else {
            frame._rotation = previousFrame._rotation;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._scale;
        } else {
            frame._scale = previousFrame._scale;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._headRotation.x >> frame._headRotation.y >> frame._headRotation.z >> frame._headRotation.w;
        } else {
            frame._headRotation = previousFrame._headRotation;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._leanSideways;
        } else {
            frame._leanSideways = previousFrame._leanSideways;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._leanForward;
        } else {
            frame._leanForward = previousFrame._leanForward;
        }
        
        recording->_frames << frame;
    }
    
    QByteArray audioArray;
    fileStream >> audioArray;
    recording->addAudioPacket(audioArray);
    
    
    qDebug() << "Read " << byteArray.size()  << " bytes in " << timer.elapsed() << " ms.";
    return recording;
}


