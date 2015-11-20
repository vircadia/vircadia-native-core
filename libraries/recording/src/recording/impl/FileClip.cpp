//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileClip.h"

#include <algorithm>

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <Finally.h>

#include "../Frame.h"
#include "../Logging.h"
#include "BufferClip.h"


using namespace recording;

FileClip::FileClip(const QString& fileName) : _file(fileName) {
    auto size = _file.size();
    qDebug() << "Opening file of size: " << size;
    bool opened = _file.open(QIODevice::ReadOnly);
    if (!opened) {
        qCWarning(recordingLog) << "Unable to open file " << fileName;
        return;
    }
    auto mappedFile = _file.map(0, size, QFile::MapPrivateOption);
    init(mappedFile, size);
}


QString FileClip::getName() const {
    return _file.fileName();
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
    //qDebug() << "Writing frame with time offset " << frame.timeOffset;
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

bool FileClip::write(const QString& fileName, Clip::Pointer clip) {
    // FIXME need to move this to a different thread
    //qCDebug(recordingLog) << "Writing clip to file " << fileName << " with " << clip->frameCount() << " frames";

    if (0 == clip->frameCount()) {
        return false;
    }

    QFile outputFile(fileName);
    if (!outputFile.open(QFile::Truncate | QFile::WriteOnly)) {
        return false;
    }

    Finally closer([&] { outputFile.close(); });
    {
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
        if (!writeFrame(outputFile, Frame({ Frame::TYPE_HEADER, 0, headerFrameData }), false)) {
            return false;
        }

    }

    clip->seek(0);
    for (auto frame = clip->nextFrame(); frame; frame = clip->nextFrame()) {
        if (!writeFrame(outputFile, *frame)) {
            return false;
        }
    }
    outputFile.close();
    return true;
}

FileClip::~FileClip() {
    Locker lock(_mutex);
    _file.unmap(_data);
    if (_file.isOpen()) {
        _file.close();
    }
    reset();
}
