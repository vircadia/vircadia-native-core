//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Clip.h"

#include "Frame.h"
#include "Logging.h"

#include "impl/FileClip.h"
#include "impl/BufferClip.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QBuffer>
#include <QtCore/QDebug>

using namespace recording;

Clip::Pointer Clip::fromFile(const QString& filePath) {
    auto result = std::make_shared<FileClip>(filePath);
    if (result->frameCount() == 0) {
        return Clip::Pointer();
    }
    return result;
}

void Clip::toFile(const QString& filePath, const Clip::ConstPointer& clip) {
    FileClip::write(filePath, clip->duplicate());
}

QByteArray Clip::toBuffer(const Clip::ConstPointer& clip) {
    QBuffer buffer;
    if (buffer.open(QFile::Truncate | QFile::WriteOnly)) {
        clip->duplicate()->write(buffer);
        buffer.close();
    }
    return buffer.data();
}

Clip::Pointer Clip::newClip() {
    return std::make_shared<BufferClip>();
}

void Clip::seek(float offset) {
    seekFrameTime(Frame::secondsToFrameTime(offset));
}

float Clip::position() const {
    return Frame::frameTimeToSeconds(positionFrameTime());
}

// FIXME move to frame?
bool writeFrame(QIODevice& output, const Frame& frame, bool compressed = true) {
    if (frame.type == Frame::TYPE_INVALID) {
        qWarning() << "Attempting to write invalid frame";
        return true;
    }

    auto written = output.write((char*)&(frame.type), sizeof(FrameType));
    if (written != sizeof(FrameType)) {
        return false;
    }
    //qDebug(recordingLog) << "Writing frame with time offset " << frame.timeOffset;
    written = output.write((char*)&(frame.timeOffset), sizeof(Frame::Time));
    if (written != sizeof(Frame::Time)) {
        return false;
    }
    QByteArray frameData = frame.data;
    if (compressed) {
        frameData = qCompress(frameData);
    }

    uint16_t dataSize = frameData.size();
    written = output.write((char*)&dataSize, sizeof(FrameSize));
    if (written != sizeof(uint16_t)) {
        return false;
    }

    if (dataSize != 0) {
        written = output.write(frameData);
        if (written != dataSize) {
            return false;
        }
    }
    return true;
}

const QString Clip::FRAME_TYPE_MAP = QStringLiteral("frameTypes");
const QString Clip::FRAME_COMREPSSION_FLAG = QStringLiteral("compressed");

bool Clip::write(QIODevice& output) {
    auto frameTypes = Frame::getFrameTypes();
    QJsonObject frameTypeObj;
    for (const auto& frameTypeName : frameTypes.keys()) {
        frameTypeObj[frameTypeName] = frameTypes[frameTypeName];
    }

    QJsonObject rootObject;
    rootObject.insert(FRAME_TYPE_MAP, frameTypeObj);
    // Always mark new files as compressed
    rootObject.insert(FRAME_COMREPSSION_FLAG, true);
    QByteArray headerFrameData = QJsonDocument(rootObject).toBinaryData();
    // Never compress the header frame
    if (!writeFrame(output, Frame({ Frame::TYPE_HEADER, 0, headerFrameData }), false)) {
        return false;
    }

    seek(0);

    for (auto frame = nextFrame(); frame; frame = nextFrame()) {
        if (!writeFrame(output, *frame)) {
            return false;
        }
    }
    return true;
}
