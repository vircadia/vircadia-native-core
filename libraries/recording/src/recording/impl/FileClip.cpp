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

static const qint64 MINIMUM_FRAME_SIZE = sizeof(FrameType) + sizeof(Frame::Time) + sizeof(FrameSize);
static const QString FRAME_TYPE_MAP = QStringLiteral("frameTypes");
static const QString FRAME_COMREPSSION_FLAG = QStringLiteral("compressed");

using FrameTranslationMap = QMap<FrameType, FrameType>;

FrameTranslationMap parseTranslationMap(const QJsonDocument& doc) {
    FrameTranslationMap results;
    auto headerObj = doc.object();
    if (headerObj.contains(FRAME_TYPE_MAP)) {
        auto frameTypeObj = headerObj[FRAME_TYPE_MAP].toObject();
        auto currentFrameTypes = Frame::getFrameTypes();
        for (auto frameTypeName : frameTypeObj.keys()) {
            qDebug() << frameTypeName;
            if (!currentFrameTypes.contains(frameTypeName)) {
                continue;
            }
            FrameType currentTypeEnum = currentFrameTypes[frameTypeName];
            FrameType storedTypeEnum = static_cast<FrameType>(frameTypeObj[frameTypeName].toInt());
            results[storedTypeEnum] = currentTypeEnum;
        }
    }
    return results;
}


FileFrameHeaderList parseFrameHeaders(uchar* const start, const qint64& size) {
    FileFrameHeaderList results;
    auto current = start;
    auto end = current + size;
    // Read all the frame headers
    // FIXME move to Frame::readHeader?
    while (end - current >= MINIMUM_FRAME_SIZE) {
        FileFrameHeader header;
        memcpy(&(header.type), current, sizeof(FrameType));
        current += sizeof(FrameType);
        memcpy(&(header.timeOffset), current, sizeof(Frame::Time));
        current += sizeof(Frame::Time);
        memcpy(&(header.size), current, sizeof(FrameSize));
        current += sizeof(FrameSize);
        header.fileOffset = current - start;
        if (end - current < header.size) {
            current = end;
            break;
        }
        current += header.size;
        results.push_back(header);
    }
    qDebug() << "Parsed source data into " << results.size() << " frames";
    int i = 0;
    for (const auto& frameHeader : results) {
        qDebug() << "Frame " << i++ << " time " << frameHeader.timeOffset;
    }
    return results;
}


FileClip::FileClip(const QString& fileName) : _file(fileName) {
    auto size = _file.size();
    bool opened = _file.open(QIODevice::ReadOnly);
    if (!opened) {
        qCWarning(recordingLog) << "Unable to open file " << fileName;
        return;
    }
    _map = _file.map(0, size, QFile::MapPrivateOption);
    if (!_map) {
        qCWarning(recordingLog) << "Unable to map file " << fileName;
        return;
    }

    auto parsedFrameHeaders = parseFrameHeaders(_map, size);

    // Verify that at least one frame exists and that the first frame is a header
    if (0 == parsedFrameHeaders.size()) {
        qWarning() << "No frames found, invalid file";
        return;
    }

    // Grab the file header
    {
        auto fileHeaderFrameHeader = *parsedFrameHeaders.begin();
        parsedFrameHeaders.pop_front();
        if (fileHeaderFrameHeader.type != Frame::TYPE_HEADER) {
            qWarning() << "Missing header frame, invalid file";
            return;
        }

        QByteArray fileHeaderData((char*)_map + fileHeaderFrameHeader.fileOffset, fileHeaderFrameHeader.size);
        _fileHeader = QJsonDocument::fromBinaryData(fileHeaderData);
    }

    // Check for compression
    {
        _compressed = _fileHeader.object()[FRAME_COMREPSSION_FLAG].toBool();
    }

    // Find the type enum translation map and fix up the frame headers
    {
        FrameTranslationMap translationMap = parseTranslationMap(_fileHeader);
        if (translationMap.empty()) {
            qWarning() << "Header missing frame type map, invalid file";
            return;
        }
        qDebug() << translationMap;

        // Update the loaded headers with the frame data
        _frames.reserve(parsedFrameHeaders.size());
        for (auto& frameHeader : parsedFrameHeaders) {
            if (!translationMap.contains(frameHeader.type)) {
                continue;
            }
            frameHeader.type = translationMap[frameHeader.type];
            _frames.push_back(frameHeader);
        }
    }

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
    _file.unmap(_map);
    _map = nullptr;
    if (_file.isOpen()) {
        _file.close();
    }
}

// Internal only function, needs no locking
FrameConstPointer FileClip::readFrame(size_t frameIndex) const {
    FramePointer result;
    if (frameIndex < _frames.size()) {
        result = std::make_shared<Frame>();
        const auto& header = _frames[frameIndex];
        result->type = header.type;
        result->timeOffset = header.timeOffset;
        if (header.size) {
            result->data.insert(0, reinterpret_cast<char*>(_map)+header.fileOffset, header.size);
            if (_compressed) {
                result->data = qUncompress(result->data);
            }
        }
    }
    return result;
}

void FileClip::addFrame(FrameConstPointer) {
    throw std::runtime_error("File clips are read only");
}
