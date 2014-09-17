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

// HFR file format magic number
// (decimal)               17  72  70  82  13  10  26  10
// (hexadecimal)           11  48  46  52  0d  0a  1a  0a
// (ASCII C notation)    \021   H   F   R  \r  \n \032 \n
static const int MAGIC_NUMBER_SIZE = 8;
static const char MAGIC_NUMBER[MAGIC_NUMBER_SIZE] = {17, 72, 70, 82, 13, 10, 26, 10};
// Version (Major, Minor)
static const QPair<quint8, quint8> VERSION(0, 1);

int SCALE_RADIX = 10;
int BLENDSHAPE_RADIX = 15;
int LEAN_RADIX = 7;

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
        return;
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
    _loop(false),
    _useAttachments(true),
    _useDisplayName(true),
    _useHeadURL(true),
    _useSkeletonURL(true)
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
        _currentContext.globalTimestamp = usecTimestampNow();
        _currentContext.domain = NodeList::getInstance()->getDomainHandler().getHostname();
        _currentContext.position = _avatar->getPosition();
        _currentContext.orientation = _avatar->getOrientation();
        _currentContext.scale = _avatar->getTargetScale();
        _currentContext.headModel = _avatar->getFaceModelURL().toString();
        _currentContext.skeletonModel = _avatar->getSkeletonModelURL().toString();
        _currentContext.displayName = _avatar->getDisplayName();
        _currentContext.attachments = _avatar->getAttachmentData();
        
        _currentContext.orientationInv = glm::inverse(_currentContext.orientation);
        
        RecordingContext& context = _recording->getContext();
        if (_useAttachments) {
            _avatar->setAttachmentData(context.attachments);
        }
        if (_useDisplayName) {
            _avatar->setDisplayName(context.displayName);
        }
        if (_useHeadURL) {
            _avatar->setFaceModelURL(context.headModel);
        }
        if (_useSkeletonURL) {
            _avatar->setSkeletonModelURL(context.skeletonModel);
        }
        
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
        
        // Fake faceshift connection
        _avatar->setForceFaceshiftConnected(true);
        
        qDebug() << "Recorder::startPlaying()";
        _currentFrame = 0;
        setupAudioThread();
        _timer.start();
    }
}

void Player::stopPlaying() {
    if (!isPlaying()) {
        return;
    }
    _timer.invalidate();
    cleanupAudioThread();
    _avatar->clearJointsData();
    
    // Turn off fake faceshift connection
    _avatar->setForceFaceshiftConnected(false);
    
    if (_useAttachments) {
        _avatar->setAttachmentData(_currentContext.attachments);
    }
    if (_useDisplayName) {
        _avatar->setDisplayName(_currentContext.displayName);
    }
    if (_useHeadURL) {
        _avatar->setFaceModelURL(_currentContext.headModel);
    }
    if (_useSkeletonURL) {
        _avatar->setSkeletonModelURL(_currentContext.skeletonModel);
    }
    
    qDebug() << "Recorder::stopPlaying()";
}

void Player::setupAudioThread() {
    _audioThread = new QThread();
    _options.setPosition(_avatar->getPosition());
    _options.setOrientation(_avatar->getOrientation());
    _injector.reset(new AudioInjector(_recording->getAudio(), _options), &QObject::deleteLater);
    _injector->moveToThread(_audioThread);
    _audioThread->start();
    QMetaObject::invokeMethod(_injector.data(), "injectAudio", Qt::QueuedConnection);
}

void Player::cleanupAudioThread() {
    _injector->stop();
    QObject::connect(_injector.data(), &AudioInjector::finished,
                     _injector.data(), &AudioInjector::deleteLater);
    QObject::connect(_injector.data(), &AudioInjector::destroyed,
                     _audioThread, &QThread::quit);
    QObject::connect(_audioThread, &QThread::finished,
                     _audioThread, &QThread::deleteLater);
    _injector.clear();
    _audioThread = NULL;
}

void Player::loopRecording() {
    cleanupAudioThread();
    setupAudioThread();
    _currentFrame = 0;
    _timer.restart();
    
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
        if (_loop) {
            loopRecording();
        } else {
            stopPlaying();
        }
        return;
    }
    
    const RecordingContext* context = &_recording->getContext();
    if (_playFromCurrentPosition) {
        context = &_currentContext;
    }
    const RecordingFrame& currentFrame = _recording->getFrame(_currentFrame);
    
    _avatar->setPosition(context->position + context->orientation * currentFrame.getTranslation());
    _avatar->setOrientation(context->orientation * currentFrame.getRotation());
    _avatar->setTargetScale(context->scale * currentFrame.getScale());
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
    unsigned char buffer[sizeof(value)];
    memcpy(buffer, &value, sizeof(value));
    stream.writeRawData(reinterpret_cast<char*>(buffer), sizeof(value));
}

bool readVec3(QDataStream& stream, glm::vec3& value) {
    unsigned char buffer[sizeof(value)];
    stream.readRawData(reinterpret_cast<char*>(buffer), sizeof(value));
    memcpy(&value, buffer, sizeof(value));
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

void writeFloat(QDataStream& stream, float value, int radix) {
    unsigned char buffer[256];
    int writtenToBuffer = packFloatScalarToSignedTwoByteFixed(buffer, value, radix);
    stream.writeRawData(reinterpret_cast<char*>(buffer), writtenToBuffer);
}

bool readFloat(QDataStream& stream, float& value, int radix) {
    int floatByteSize = 2; // 1 floats * 2 bytes
    int16_t buffer[256];
    stream.readRawData(reinterpret_cast<char*>(buffer), floatByteSize);
    int readFromBuffer = unpackFloatScalarFromSignedTwoByteFixed(buffer, &value, radix);
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
    
    QElapsedTimer timer;
    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)){
        qDebug() << "Couldn't open " << filename;
        return;
    }
    timer.start();
    qDebug() << "Writing recording to " << filename << ".";
    
    QDataStream fileStream(&file);
    
    // HEADER
    file.write(MAGIC_NUMBER, MAGIC_NUMBER_SIZE); // Magic number
    fileStream << VERSION; // File format version
    const qint64 dataOffsetPos = file.pos();
    fileStream << (quint16)0; // Save two empty bytes for the data offset
    const qint64 dataLengthPos = file.pos();
    fileStream << (quint32)0; // Save four empty bytes for the data offset
    const quint64 crc16Pos = file.pos();
    fileStream << (quint16)0; // Save two empty bytes for the CRC-16
    
    
    // METADATA
    // TODO
    
    
    
    // Write data offset
    quint16 dataOffset = file.pos();
    file.seek(dataOffsetPos);
    fileStream << dataOffset;
    file.seek(dataOffset);

    // CONTEXT
    RecordingContext& context = recording->getContext();
    // Global Timestamp
    fileStream << context.globalTimestamp;
    // Domain
    fileStream << context.domain;
    // Position
    writeVec3(fileStream, context.position);
    // Orientation
    writeQuat(fileStream, context.orientation);
    // Scale
    writeFloat(fileStream, context.scale, SCALE_RADIX);
    // Head model
    fileStream << context.headModel;
    // Skeleton model
    fileStream << context.skeletonModel;
    // Display name
    fileStream << context.displayName;
    // Attachements
    fileStream << (quint8)context.attachments.size();
    foreach (AttachmentData data, context.attachments) {
        // Model
        fileStream << data.modelURL.toString();
        // Joint name
        fileStream << data.jointName;
        // Position
        writeVec3(fileStream, data.translation);
        // Orientation
        writeQuat(fileStream, data.rotation);
        // Scale
        writeFloat(fileStream, data.scale, SCALE_RADIX);
    }
    
    // RECORDING
    fileStream << recording->_timestamps;
    
    QBitArray mask;
    quint32 numBlendshapes = 0;
    quint32 numJoints = 0;
    
    for (int i = 0; i < recording->_timestamps.size(); ++i) {
        mask.fill(false);
        int maskIndex = 0;
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::WriteOnly);
        RecordingFrame& previousFrame = recording->_frames[(i != 0) ? i - 1 : i];
        RecordingFrame& frame = recording->_frames[i];
        
        // Blendshape Coefficients
        if (i == 0) {
            numBlendshapes = frame.getBlendshapeCoefficients().size();
            stream << numBlendshapes;
            mask.resize(mask.size() + numBlendshapes);
        }
        for (int j = 0; j < numBlendshapes; ++j) {
            if (i == 0 ||
                frame._blendshapeCoefficients[j] != previousFrame._blendshapeCoefficients[j]) {
                writeFloat(stream, frame.getBlendshapeCoefficients()[j], BLENDSHAPE_RADIX);
                mask.setBit(maskIndex);
            }
            ++maskIndex;
        }
        
        // Joint Rotations
        if (i == 0) {
            numJoints = frame.getJointRotations().size();
            stream << numJoints;
            mask.resize(mask.size() + numJoints);
        }
        for (int j = 0; j < numJoints; ++j) {
            if (i == 0 ||
                frame._jointRotations[j] != previousFrame._jointRotations[j]) {
                writeQuat(stream, frame._jointRotations[j]);
                mask.setBit(maskIndex);
            }
            maskIndex++;
        }
        
        // Translation
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._translation != previousFrame._translation) {
            writeVec3(stream, frame._translation);
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Rotation
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._rotation != previousFrame._rotation) {
            writeQuat(stream, frame._rotation);
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Scale
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._scale != previousFrame._scale) {
            writeFloat(stream, frame._scale, SCALE_RADIX);
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Head Rotation
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._headRotation != previousFrame._headRotation) {
            writeQuat(stream, frame._headRotation);
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Lean Sideways
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._leanSideways != previousFrame._leanSideways) {
            writeFloat(stream, frame._leanSideways, LEAN_RADIX);
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Lean Forward
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._leanForward != previousFrame._leanForward) {
            writeFloat(stream, frame._leanForward, LEAN_RADIX);
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // LookAt Position
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._lookAtPosition != previousFrame._lookAtPosition) {
            writeVec3(stream, frame._lookAtPosition);
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        fileStream << mask;
        fileStream << buffer;
    }
    
    fileStream << recording->_audio->getByteArray();
    
    qint64 writtingTime = timer.restart();
    // Write data length and CRC-16
    quint32 dataLength = file.pos() - dataOffset;
    file.seek(dataOffset); // Go to beginning of data for checksum
    quint16 crc16 = qChecksum(file.readAll().constData(), dataLength);
    
    file.seek(dataLengthPos);
    fileStream << dataLength;
    file.seek(crc16Pos);
    fileStream << crc16;
    file.seek(dataOffset + dataLength);
    
    bool wantDebug = true;
    if (wantDebug) {
        qDebug() << "[DEBUG] WRITE recording";
        qDebug() << "Header:";
        qDebug() << "File Format version:" << VERSION;
        qDebug() << "Data length:" << dataLength;
        qDebug() << "Data offset:" << dataOffset;
        qDebug() << "CRC-16:" << crc16;
        
        qDebug() << "Context block:";
        qDebug() << "Global timestamp:" << context.globalTimestamp;
        qDebug() << "Domain:" << context.domain;
        qDebug() << "Position:" << context.position;
        qDebug() << "Orientation:" << context.orientation;
        qDebug() << "Scale:" << context.scale;
        qDebug() << "Head Model:" << context.headModel;
        qDebug() << "Skeleton Model:" << context.skeletonModel;
        qDebug() << "Display Name:" << context.displayName;
        qDebug() << "Num Attachments:" << context.attachments.size();
        for (int i = 0; i < context.attachments.size(); ++i) {
            qDebug() << "Model URL:" << context.attachments[i].modelURL;
            qDebug() << "Joint Name:" << context.attachments[i].jointName;
            qDebug() << "Translation:" << context.attachments[i].translation;
            qDebug() << "Rotation:" << context.attachments[i].rotation;
            qDebug() << "Scale:" << context.attachments[i].scale;
        }
        
        qDebug() << "Recording:";
        qDebug() << "Total frames:" << recording->getFrameNumber();
        qDebug() << "Audio array:" << recording->getAudio()->getByteArray().size();
    }
    
    qint64 checksumTime = timer.elapsed();
    qDebug() << "Wrote" << file.size() << "bytes in" << writtingTime + checksumTime << "ms. (" << checksumTime << "ms for checksum)";
}

RecordingPointer readRecordingFromFile(RecordingPointer recording, QString filename) {
    QByteArray byteArray;
    QUrl url(filename);
    QElapsedTimer timer;
    timer.start(); // timer used for debug informations (download/parsing time)
    
    // Aquire the data and place it in byteArray
    // Return if data unavailable
    if (url.scheme() == "http" || url.scheme() == "https" || url.scheme() == "ftp") {
        // Download file if necessary
        qDebug() << "Downloading recording at" << url;
        NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkReply* reply = networkAccessManager.get(QNetworkRequest(url));
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec(); // wait for file
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error while downloading recording: " << reply->error();
            reply->deleteLater();
            return recording;
        }
        byteArray = reply->readAll();
        reply->deleteLater();
        // print debug + restart timer
        qDebug() << "Downloaded " << byteArray.size() << " bytes in " << timer.restart() << " ms.";
    } else {
        // If local file, just read it.
        qDebug() << "Reading recording from " << filename << ".";
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)){
            qDebug() << "Could not open local file: " << url;
            return recording;
        }
        byteArray = file.readAll();
        file.close();
    }
    
    // Reset the recording passed in the arguments
    if (!recording) {
        recording.reset(new Recording());
    }
    
    QDataStream fileStream(byteArray);
    
    // HEADER
    QByteArray magicNumber(MAGIC_NUMBER, MAGIC_NUMBER_SIZE);
    if (!byteArray.startsWith(magicNumber)) {
        qDebug() << "ERROR: This is not a .HFR file. (Magic Number incorrect)";
        return recording;
    }
    fileStream.skipRawData(MAGIC_NUMBER_SIZE);
    
    QPair<quint8, quint8> version;
    fileStream >> version; // File format version
    if (version != VERSION) {
        qDebug() << "ERROR: This file format version is not supported.";
        return recording;
    }
    
    quint16 dataOffset = 0;
    fileStream >> dataOffset;
    quint32 dataLength = 0;
    fileStream >> dataLength;
    quint16 crc16 = 0;
    fileStream >> crc16;
    
    
    // Check checksum
    
    quint16 computedCRC16 = qChecksum(byteArray.constData() + dataOffset, dataLength);
    if (computedCRC16 != crc16) {
        qDebug() << "Checksum does not match. Bailling!";
        recording.clear();
        return recording;
    }
    
    // METADATA
    // TODO
    
    
    
    // CONTEXT
    RecordingContext& context = recording->getContext();
    // Global Timestamp
    fileStream >> context.globalTimestamp;
    // Domain
    fileStream >> context.domain;
    // Position
    if (!readVec3(fileStream, context.position)) {
        qDebug() << "Couldn't read file correctly. (Invalid vec3)";
        recording.clear();
        return recording;
    }
    // Orientation
    if (!readQuat(fileStream, context.orientation)) {
        qDebug() << "Couldn't read file correctly. (Invalid quat)";
        recording.clear();
        return recording;
    }

    // Scale
    if (!readFloat(fileStream, context.scale, SCALE_RADIX)) {
        qDebug() << "Couldn't read file correctly. (Invalid float)";
        recording.clear();
        return recording;
    }
    // Head model
    fileStream >> context.headModel;
    // Skeleton model
    fileStream >> context.skeletonModel;
    // Display Name
    fileStream >> context.displayName;
    
    // Attachements
    quint8 numAttachments = 0;
    fileStream >> numAttachments;
    for (int i = 0; i < numAttachments; ++i) {
        AttachmentData data;
        // Model
        QString modelURL;
        fileStream >> modelURL;
        data.modelURL = modelURL;
        // Joint name
        fileStream >> data.jointName;
        // Translation
        if (!readVec3(fileStream, data.translation)) {
            qDebug() << "Couldn't read attachment correctly. (Invalid vec3)";
            continue;
        }
        // Rotation
        if (!readQuat(fileStream, data.rotation)) {
            qDebug() << "Couldn't read attachment correctly. (Invalid quat)";
            continue;
        }
        
        // Scale
        if (!readFloat(fileStream, data.scale, SCALE_RADIX)) {
            qDebug() << "Couldn't read attachment correctly. (Invalid float)";
            continue;
        }
        context.attachments << data;
    }
    
    quint32 numBlendshapes = 0;
    quint32 numJoints = 0;
    // RECORDING
    fileStream >> recording->_timestamps;
    
    for (int i = 0; i < recording->_timestamps.size(); ++i) {
        QBitArray mask;
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::ReadOnly);
        RecordingFrame frame;
        RecordingFrame& previousFrame = (i == 0) ? frame : recording->_frames.last();
        
        fileStream >> mask;
        fileStream >> buffer;
        int maskIndex = 0;
        
        // Blendshape Coefficients
        if (i == 0) {
            stream >> numBlendshapes;
        }
        frame._blendshapeCoefficients.resize(numBlendshapes);
        for (int j = 0; j < numBlendshapes; ++j) {
            if (!mask[maskIndex++] || !readFloat(stream, frame._blendshapeCoefficients[j], BLENDSHAPE_RADIX)) {
                frame._blendshapeCoefficients[j] = previousFrame._blendshapeCoefficients[j];
            }
        }
        // Joint Rotations
        if (i == 0) {
            stream >> numJoints;
        }
        frame._jointRotations.resize(numJoints);
        for (int j = 0; j < numJoints; ++j) {
            if (!mask[maskIndex++] || !readQuat(stream, frame._jointRotations[j])) {
                frame._jointRotations[j] = previousFrame._jointRotations[j];
            }
        }
        
        if (!mask[maskIndex++] || !readVec3(stream, frame._translation)) {
            frame._translation = previousFrame._translation;
        }
        
        if (!mask[maskIndex++] || !readQuat(stream, frame._rotation)) {
            frame._rotation = previousFrame._rotation;
        }
        
        if (!mask[maskIndex++] || !readFloat(stream, frame._scale, SCALE_RADIX)) {
            frame._scale = previousFrame._scale;
        }
        
        if (!mask[maskIndex++] || !readQuat(stream, frame._headRotation)) {
            frame._headRotation = previousFrame._headRotation;
        }
        
        if (!mask[maskIndex++] || !readFloat(stream, frame._leanSideways, LEAN_RADIX)) {
            frame._leanSideways = previousFrame._leanSideways;
        }
        
        if (!mask[maskIndex++] || !readFloat(stream, frame._leanForward, LEAN_RADIX)) {
            frame._leanForward = previousFrame._leanForward;
        }
        
        if (!mask[maskIndex++] || !readVec3(stream, frame._lookAtPosition)) {
            frame._lookAtPosition = previousFrame._lookAtPosition;
        }
        
        recording->_frames << frame;
    }
    
    QByteArray audioArray;
    fileStream >> audioArray;
    recording->addAudioPacket(audioArray);
    
    bool wantDebug = true;
    if (wantDebug) {
        qDebug() << "[DEBUG] READ recording";
        qDebug() << "Header:";
        qDebug() << "File Format version:" << VERSION;
        qDebug() << "Data length:" << dataLength;
        qDebug() << "Data offset:" << dataOffset;
        qDebug() << "CRC-16:" << crc16;
        
        qDebug() << "Context block:";
        qDebug() << "Global timestamp:" << context.globalTimestamp;
        qDebug() << "Domain:" << context.domain;
        qDebug() << "Position:" << context.position;
        qDebug() << "Orientation:" << context.orientation;
        qDebug() << "Scale:" << context.scale;
        qDebug() << "Head Model:" << context.headModel;
        qDebug() << "Skeleton Model:" << context.skeletonModel;
        qDebug() << "Display Name:" << context.displayName;
        qDebug() << "Num Attachments:" << numAttachments;
        for (int i = 0; i < numAttachments; ++i) {
            qDebug() << "Model URL:" << context.attachments[i].modelURL;
            qDebug() << "Joint Name:" << context.attachments[i].jointName;
            qDebug() << "Translation:" << context.attachments[i].translation;
            qDebug() << "Rotation:" << context.attachments[i].rotation;
            qDebug() << "Scale:" << context.attachments[i].scale;
        }
        
        qDebug() << "Recording:";
        qDebug() << "Total frames:" << recording->getFrameNumber();
        qDebug() << "Audio array:" << recording->getAudio()->getByteArray().size();
        
    }
    
    qDebug() << "Read " << byteArray.size()  << " bytes in " << timer.elapsed() << " ms.";
    return recording;
}


