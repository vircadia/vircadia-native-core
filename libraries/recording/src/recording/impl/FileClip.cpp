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


using namespace recording;

static const qint64 MINIMUM_FRAME_SIZE = sizeof(FrameType) + sizeof(Time) + sizeof(FrameSize);

static const QString FRAME_TYPE_MAP = QStringLiteral("frameTypes");

using FrameHeaderList = std::list<FileClip::FrameHeader>;
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


FrameHeaderList parseFrameHeaders(uchar* const start, const qint64& size) {
    using FrameHeader = FileClip::FrameHeader;
    FrameHeaderList results;
    auto current = start;
    auto end = current + size;
    // Read all the frame headers
    // FIXME move to Frame::readHeader?
    while (end - current >= MINIMUM_FRAME_SIZE) {
        FrameHeader header;
        memcpy(&(header.type), current, sizeof(FrameType));
        current += sizeof(FrameType);
        memcpy(&(header.timeOffset), current, sizeof(Time));
        current += sizeof(Time);
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

    FrameHeaderList parsedFrameHeaders = parseFrameHeaders(_map, size);

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

    // Find the type enum translation map and fix up the frame headers
    {
        FrameTranslationMap translationMap = parseTranslationMap(_fileHeader);
        if (translationMap.empty()) {
            qWarning() << "Header missing frame type map, invalid file";
            return;
        }
        qDebug() << translationMap;

        // Update the loaded headers with the frame data
        _frameHeaders.reserve(parsedFrameHeaders.size());
        for (auto& frameHeader : parsedFrameHeaders) {
            if (!translationMap.contains(frameHeader.type)) {
                continue;
            }
            frameHeader.type = translationMap[frameHeader.type];
            _frameHeaders.push_back(frameHeader);
        }
    }
}

// FIXME move to frame?
bool writeFrame(QIODevice& output, const Frame& frame) {
    if (frame.type == Frame::TYPE_INVALID) {
        qWarning() << "Attempting to write invalid frame";
        return true;
    }

    auto written = output.write((char*)&(frame.type), sizeof(FrameType));
    if (written != sizeof(FrameType)) {
        return false;
    }
    written = output.write((char*)&(frame.timeOffset), sizeof(Time));
    if (written != sizeof(Time)) {
        return false;
    }
    uint16_t dataSize = frame.data.size();
    written = output.write((char*)&dataSize, sizeof(FrameSize));
    if (written != sizeof(uint16_t)) {
        return false;
    }
    if (dataSize != 0) {
        written = output.write(frame.data);
        if (written != dataSize) {
            return false;
        }
    }
    return true;
}

bool FileClip::write(const QString& fileName, Clip::Pointer clip) {
    qCDebug(recordingLog) << "Writing clip to file " << fileName;

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
        QByteArray headerFrameData = QJsonDocument(rootObject).toBinaryData();
        if (!writeFrame(outputFile, Frame({ Frame::TYPE_HEADER, 0, headerFrameData }))) {
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

void FileClip::seek(Time offset) {
    Locker lock(_mutex);
    auto itr = std::lower_bound(_frameHeaders.begin(), _frameHeaders.end(), offset,
        [](const FrameHeader& a, Time b)->bool {
            return a.timeOffset < b;
        }
    );
    _frameIndex = itr - _frameHeaders.begin();
}

Time FileClip::position() const {
    Locker lock(_mutex);
    Time result = INVALID_TIME;
    if (_frameIndex < _frameHeaders.size()) {
        result = _frameHeaders[_frameIndex].timeOffset;
    }
    return result;
}

FramePointer FileClip::readFrame(uint32_t frameIndex) const {
    FramePointer result;
    if (frameIndex < _frameHeaders.size()) {
        result = std::make_shared<Frame>();
        const FrameHeader& header = _frameHeaders[frameIndex];
        result->type = header.type;
        result->timeOffset = header.timeOffset;
        if (header.size) {
            result->data.insert(0, reinterpret_cast<char*>(_map)+header.fileOffset, header.size);
        }
    }
    return result;
}

FramePointer FileClip::peekFrame() const {
    Locker lock(_mutex);
    return readFrame(_frameIndex);
}

FramePointer FileClip::nextFrame() {
    Locker lock(_mutex);
    auto result = readFrame(_frameIndex);
    if (_frameIndex < _frameHeaders.size()) {
        ++_frameIndex;
    }
    return result;
}

void FileClip::skipFrame() {
    ++_frameIndex;
}

void FileClip::reset() {
    _frameIndex = 0;
}

void FileClip::addFrame(FramePointer) {
    throw std::runtime_error("File clips are read only");
}

Time FileClip::duration() const {
    if (_frameHeaders.empty()) {
        return 0;
    }
    return _frameHeaders.rbegin()->timeOffset;
}

size_t FileClip::frameCount() const {
    return _frameHeaders.size();
}

