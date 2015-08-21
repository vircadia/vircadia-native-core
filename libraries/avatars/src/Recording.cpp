//
//  Recording.cpp
//
//
//  Created by Clement on 9/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AudioConstants.h>
#include <GLMHelpers.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <Sound.h>
#include <StreamUtils.h>

#include <QBitArray>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QPair>

#include "AvatarData.h"
#include "AvatarLogging.h"
#include "Recording.h"

// HFR file format magic number (Inspired by PNG)
// (decimal)               17  72  70  82  13  10  26  10
// (hexadecimal)           11  48  46  52  0d  0a  1a  0a
// (ASCII C notation)    \021   H   F   R  \r  \n \032 \n
static const int MAGIC_NUMBER_SIZE = 8;
static const char MAGIC_NUMBER[MAGIC_NUMBER_SIZE] = {17, 72, 70, 82, 13, 10, 26, 10};
// Version (Major, Minor)
static const QPair<quint8, quint8> VERSION(0, 2);

int SCALE_RADIX = 10;
int BLENDSHAPE_RADIX = 15;
int LEAN_RADIX = 7;

void RecordingFrame::setBlendshapeCoefficients(QVector<float> blendshapeCoefficients) {
    _blendshapeCoefficients = blendshapeCoefficients;
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
    if (i < 0) {
        return 0;
    }
    return _timestamps[i];
}

const RecordingFrame& Recording::getFrame(int i) const {
    assert(i < _timestamps.size());
    return _frames[i];
}


int Recording::numberAudioChannel() const {
    // Check for stereo audio
    int MSEC_PER_SEC = 1000;
    int channelLength = (getLength() / MSEC_PER_SEC) *
                        AudioConstants::SAMPLE_RATE * sizeof(AudioConstants::AudioSample);
    return glm::round((float)channelLength / (float)getAudioData().size());
}

void Recording::addFrame(int timestamp, RecordingFrame &frame) {
    _timestamps << timestamp;
    _frames << frame;
}

void Recording::clear() {
    _timestamps.clear();
    _frames.clear();
    _audioData.clear();
}

void writeVec3(QDataStream& stream, const glm::vec3& value) {
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

void writeQuat(QDataStream& stream, const glm::quat& value) {
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

void writeRecordingToFile(RecordingPointer recording, const QString& filename) {
    if (!recording || recording->getFrameNumber() < 1) {
        qCDebug(avatars) << "Can't save empty recording";
        return;
    }
    
    QElapsedTimer timer;
    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)){
        qCDebug(avatars) << "Couldn't open " << filename;
        return;
    }
    timer.start();
    qCDebug(avatars) << "Writing recording to " << filename << ".";
    
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
    fileStream << context.scale;
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
        fileStream << data.scale;
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
        for (quint32 j = 0; j < numBlendshapes; ++j) {
            if (i == 0 ||
                frame._blendshapeCoefficients[j] != previousFrame._blendshapeCoefficients[j]) {
                stream << frame.getBlendshapeCoefficients()[j];
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
        for (quint32 j = 0; j < numJoints; ++j) {
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
            stream << frame._scale;
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
            stream << frame._leanSideways;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Lean Forward
        if (i == 0) {
            mask.resize(mask.size() + 1);
        }
        if (i == 0 || frame._leanForward != previousFrame._leanForward) {
            stream << frame._leanForward;
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
    
    fileStream << recording->getAudioData();
    
    qint64 writingTime = timer.restart();
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
        qCDebug(avatars) << "[DEBUG] WRITE recording";
        qCDebug(avatars) << "Header:";
        qCDebug(avatars) << "File Format version:" << VERSION;
        qCDebug(avatars) << "Data length:" << dataLength;
        qCDebug(avatars) << "Data offset:" << dataOffset;
        qCDebug(avatars) << "CRC-16:" << crc16;
        
        qCDebug(avatars) << "Context block:";
        qCDebug(avatars) << "Global timestamp:" << context.globalTimestamp;
        qCDebug(avatars) << "Domain:" << context.domain;
        qCDebug(avatars) << "Position:" << context.position;
        qCDebug(avatars) << "Orientation:" << context.orientation;
        qCDebug(avatars) << "Scale:" << context.scale;
        qCDebug(avatars) << "Head Model:" << context.headModel;
        qCDebug(avatars) << "Skeleton Model:" << context.skeletonModel;
        qCDebug(avatars) << "Display Name:" << context.displayName;
        qCDebug(avatars) << "Num Attachments:" << context.attachments.size();
        for (int i = 0; i < context.attachments.size(); ++i) {
            qCDebug(avatars) << "Model URL:" << context.attachments[i].modelURL;
            qCDebug(avatars) << "Joint Name:" << context.attachments[i].jointName;
            qCDebug(avatars) << "Translation:" << context.attachments[i].translation;
            qCDebug(avatars) << "Rotation:" << context.attachments[i].rotation;
            qCDebug(avatars) << "Scale:" << context.attachments[i].scale;
        }
        
        qCDebug(avatars) << "Recording:";
        qCDebug(avatars) << "Total frames:" << recording->getFrameNumber();
        qCDebug(avatars) << "Audio array:" << recording->getAudioData().size();
    }
    
    qint64 checksumTime = timer.elapsed();
    qCDebug(avatars) << "Wrote" << file.size() << "bytes in" << writingTime + checksumTime << "ms. (" << checksumTime << "ms for checksum)";
}

RecordingPointer readRecordingFromFile(RecordingPointer recording, const QString& filename) {
    QByteArray byteArray;
    QUrl url(filename);
    QElapsedTimer timer;
    timer.start(); // timer used for debug informations (download/parsing time)
    
    // Aquire the data and place it in byteArray
    // Return if data unavailable
    if (url.scheme() == "http" || url.scheme() == "https" || url.scheme() == "ftp") {
        // Download file if necessary
        qCDebug(avatars) << "Downloading recording at" << url;
        QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkRequest networkRequest = QNetworkRequest(url);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
        QNetworkReply* reply = networkAccessManager.get(networkRequest);
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec(); // wait for file
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(avatars) << "Error while downloading recording: " << reply->error();
            reply->deleteLater();
            return recording;
        }
        byteArray = reply->readAll();
        reply->deleteLater();
        // print debug + restart timer
        qCDebug(avatars) << "Downloaded " << byteArray.size() << " bytes in " << timer.restart() << " ms.";
    } else {
        // If local file, just read it.
        qCDebug(avatars) << "Reading recording from " << filename << ".";
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)){
            qCDebug(avatars) << "Could not open local file: " << url;
            return recording;
        }
        byteArray = file.readAll();
        file.close();
    }
    
    if (filename.endsWith(".rec") || filename.endsWith(".REC")) {
        qCDebug(avatars) << "Old .rec format";
        readRecordingFromRecFile(recording, filename, byteArray);
        return recording;
    } else if (!filename.endsWith(".hfr") && !filename.endsWith(".HFR")) {
        qCDebug(avatars) << "File extension not recognized";
    }
    
    // Reset the recording passed in the arguments
    if (!recording) {
        recording = QSharedPointer<Recording>::create();
    }
    
    QDataStream fileStream(byteArray);
    
    // HEADER
    QByteArray magicNumber(MAGIC_NUMBER, MAGIC_NUMBER_SIZE);
    if (!byteArray.startsWith(magicNumber)) {
        qCDebug(avatars) << "ERROR: This is not a .HFR file. (Magic Number incorrect)";
        return recording;
    }
    fileStream.skipRawData(MAGIC_NUMBER_SIZE);
    
    QPair<quint8, quint8> version;
    fileStream >> version; // File format version
    if (version != VERSION && version != QPair<quint8, quint8>(0,1)) {
        qCDebug(avatars) << "ERROR: This file format version is not supported.";
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
        qCDebug(avatars) << "Checksum does not match. Bailling!";
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
        qCDebug(avatars) << "Couldn't read file correctly. (Invalid vec3)";
        recording.clear();
        return recording;
    }
    // Orientation
    if (!readQuat(fileStream, context.orientation)) {
        qCDebug(avatars) << "Couldn't read file correctly. (Invalid quat)";
        recording.clear();
        return recording;
    }
    
    // Scale
    if (version == QPair<quint8, quint8>(0,1)) {
        readFloat(fileStream, context.scale, SCALE_RADIX);
    } else {
        fileStream >> context.scale;
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
            qCDebug(avatars) << "Couldn't read attachment correctly. (Invalid vec3)";
            continue;
        }
        // Rotation
        if (!readQuat(fileStream, data.rotation)) {
            qCDebug(avatars) << "Couldn't read attachment correctly. (Invalid quat)";
            continue;
        }
        
        // Scale
        if (version == QPair<quint8, quint8>(0,1)) {
            readFloat(fileStream, data.scale, SCALE_RADIX);
        } else {
            fileStream >> data.scale;
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
        for (quint32 j = 0; j < numBlendshapes; ++j) {
            if (!mask[maskIndex++]) {
                frame._blendshapeCoefficients[j] = previousFrame._blendshapeCoefficients[j];
            } else if (version == QPair<quint8, quint8>(0,1)) {
                readFloat(stream, frame._blendshapeCoefficients[j], BLENDSHAPE_RADIX);
            } else {
                stream >> frame._blendshapeCoefficients[j];
            }
        }
        // Joint Rotations
        if (i == 0) {
            stream >> numJoints;
        }
        frame._jointRotations.resize(numJoints);
        for (quint32 j = 0; j < numJoints; ++j) {
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
        
        if (!mask[maskIndex++]) {
            frame._scale = previousFrame._scale;
        } else if (version == QPair<quint8, quint8>(0,1)) {
            readFloat(stream, frame._scale, SCALE_RADIX);
        } else {
            stream >> frame._scale;
        }
        
        if (!mask[maskIndex++] || !readQuat(stream, frame._headRotation)) {
            frame._headRotation = previousFrame._headRotation;
        }
        
        if (!mask[maskIndex++]) {
            frame._leanSideways = previousFrame._leanSideways;
        } else if (version == QPair<quint8, quint8>(0,1)) {
            readFloat(stream, frame._leanSideways, LEAN_RADIX);
        } else {
            stream >> frame._leanSideways;
        }
        
        if (!mask[maskIndex++]) {
            frame._leanForward = previousFrame._leanForward;
        } else if (version == QPair<quint8, quint8>(0,1)) {
            readFloat(stream, frame._leanForward, LEAN_RADIX);
        } else {
            stream >> frame._leanForward;
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
        qCDebug(avatars) << "[DEBUG] READ recording";
        qCDebug(avatars) << "Header:";
        qCDebug(avatars) << "File Format version:" << VERSION;
        qCDebug(avatars) << "Data length:" << dataLength;
        qCDebug(avatars) << "Data offset:" << dataOffset;
        qCDebug(avatars) << "CRC-16:" << crc16;
        
        qCDebug(avatars) << "Context block:";
        qCDebug(avatars) << "Global timestamp:" << context.globalTimestamp;
        qCDebug(avatars) << "Domain:" << context.domain;
        qCDebug(avatars) << "Position:" << context.position;
        qCDebug(avatars) << "Orientation:" << context.orientation;
        qCDebug(avatars) << "Scale:" << context.scale;
        qCDebug(avatars) << "Head Model:" << context.headModel;
        qCDebug(avatars) << "Skeleton Model:" << context.skeletonModel;
        qCDebug(avatars) << "Display Name:" << context.displayName;
        qCDebug(avatars) << "Num Attachments:" << numAttachments;
        for (int i = 0; i < numAttachments; ++i) {
            qCDebug(avatars) << "Model URL:" << context.attachments[i].modelURL;
            qCDebug(avatars) << "Joint Name:" << context.attachments[i].jointName;
            qCDebug(avatars) << "Translation:" << context.attachments[i].translation;
            qCDebug(avatars) << "Rotation:" << context.attachments[i].rotation;
            qCDebug(avatars) << "Scale:" << context.attachments[i].scale;
        }
        
        qCDebug(avatars) << "Recording:";
        qCDebug(avatars) << "Total frames:" << recording->getFrameNumber();
        qCDebug(avatars) << "Audio array:" << recording->getAudioData().size();
        
    }
    
    qCDebug(avatars) << "Read " << byteArray.size()  << " bytes in " << timer.elapsed() << " ms.";
    return recording;
}


RecordingPointer readRecordingFromRecFile(RecordingPointer recording, const QString& filename, const QByteArray& byteArray) {
    QElapsedTimer timer;
    timer.start();
    
    if (!recording) {
        recording = QSharedPointer<Recording>::create();
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
    
    
    // Fake context
    RecordingContext& context = recording->getContext();
    context.globalTimestamp = usecTimestampNow();
    context.domain = DependencyManager::get<NodeList>()->getDomainHandler().getHostname();
    context.position = glm::vec3(144.5f, 3.3f, 181.3f);
    context.orientation = glm::angleAxis(glm::radians(-92.5f), glm::vec3(0, 1, 0));;
    context.scale = baseFrame._scale;
    context.headModel = "http://public.highfidelity.io/models/heads/Emily_v4.fst";
    context.skeletonModel = "http://public.highfidelity.io/models/skeletons/EmilyCutMesh_A.fst";
    context.displayName = "Leslie";
    context.attachments.clear();
    AttachmentData data;
    data.modelURL = "http://public.highfidelity.io/models/attachments/fbx.fst";
    data.jointName = "RightHand" ;
    data.translation = glm::vec3(0.04f, 0.07f, 0.0f);
    data.rotation = glm::angleAxis(glm::radians(102.0f), glm::vec3(0, 1, 0));
    data.scale = 0.20f;
    context.attachments << data;
    
    context.orientationInv = glm::inverse(context.orientation);
    
    baseFrame._translation = glm::vec3();
    baseFrame._rotation = glm::quat();
    baseFrame._scale = 1.0f;
    
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
            frame._translation = context.orientationInv * frame._translation;
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
    
    // Cut down audio if necessary
    int SAMPLE_SIZE = 2; // 16 bits
    int MSEC_PER_SEC = 1000;
    int audioLength = recording->getLength() * SAMPLE_SIZE * (AudioConstants::SAMPLE_RATE / MSEC_PER_SEC);
    audioArray.chop(audioArray.size() - audioLength);
    
    recording->addAudioPacket(audioArray);
    
    qCDebug(avatars) << "Read " << byteArray.size()  << " bytes in " << timer.elapsed() << " ms.";
    
    // Set new filename
    QString newFilename = filename;
    if (newFilename.startsWith("http") || newFilename.startsWith("https") || newFilename.startsWith("ftp")) {
        newFilename = QUrl(newFilename).fileName();
    }
    if (newFilename.endsWith(".rec") || newFilename.endsWith(".REC")) {
        newFilename.chop(qstrlen(".rec"));
    }
    newFilename.append(".hfr");
    newFilename = QFileInfo(newFilename).absoluteFilePath();
    
    // Set recording to new format
    writeRecordingToFile(recording, newFilename);
    qCDebug(avatars) << "Recording has been successfully converted at" << newFilename;
    return recording;
}
