//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PointerClip.h"

#include <algorithm>

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <Finally.h>

#include "../Frame.h"
#include "../Logging.h"
#include "BufferClip.h"


using namespace recording;

using FrameTranslationMap = QMap<FrameType, FrameType>;

FrameTranslationMap parseTranslationMap(const QJsonDocument& doc) {
    FrameTranslationMap results;
    auto headerObj = doc.object();
    if (headerObj.contains(Clip::FRAME_TYPE_MAP)) {
        auto frameTypeObj = headerObj[Clip::FRAME_TYPE_MAP].toObject();
        auto currentFrameTypes = Frame::getFrameTypes();
        for (auto frameTypeName : frameTypeObj.keys()) {
            qDebug(recordingLog) << frameTypeName;
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


PointerFrameHeaderList parseFrameHeaders(uchar* const start, const size_t& size) {
    PointerFrameHeaderList results;
    auto current = start;
    auto end = current + size;
    // Read all the frame headers
    // FIXME move to Frame::readHeader?
    while (end - current >= PointerClip::MINIMUM_FRAME_SIZE) {
        PointerFrameHeader header;
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
    qDebug(recordingLog) << "Parsed source data into " << results.size() << " frames";
//    int i = 0;
//    for (const auto& frameHeader : results) {
//        qDebug(recordingLog) << "Frame " << i++ << " time " << frameHeader.timeOffset << " Type " << frameHeader.type;
//    }
    return results;
}

void PointerClip::reset() {
    _frames.clear();
    _data = nullptr;
    _size = 0;
    _header = QJsonDocument();
}

void PointerClip::init(uchar* data, size_t size) {
    reset();

    _data = data;
    _size = size;

    auto parsedFrameHeaders = parseFrameHeaders(data, size);
    // Verify that at least one frame exists and that the first frame is a header
    if (0 == parsedFrameHeaders.size()) {
        qWarning() << "No frames found, invalid file";
        reset();
        return;
    }

    // Grab the file header
    {
        auto fileHeaderFrameHeader = *parsedFrameHeaders.begin();
        parsedFrameHeaders.pop_front();
        if (fileHeaderFrameHeader.type != Frame::TYPE_HEADER) {
            qWarning() << "Missing header frame, invalid file";
            reset();
            return;
        }

        QByteArray fileHeaderData((char*)_data + fileHeaderFrameHeader.fileOffset, fileHeaderFrameHeader.size);
        _header = QJsonDocument::fromBinaryData(fileHeaderData);
    }

    // Check for compression
    {
        _compressed = _header.object()[FRAME_COMREPSSION_FLAG].toBool();
    }

    // Find the type enum translation map and fix up the frame headers
    {
        FrameTranslationMap translationMap = parseTranslationMap(_header);
        if (translationMap.empty()) {
            qWarning() << "Header missing frame type map, invalid file";
            reset();
            return;
        }

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

// Internal only function, needs no locking
FrameConstPointer PointerClip::readFrame(size_t frameIndex) const {
    FramePointer result;
    if (frameIndex < _frames.size()) {
        result = std::make_shared<Frame>();
        const auto& header = _frames[frameIndex];
        result->type = header.type;
        result->timeOffset = header.timeOffset;
        if (header.size) {
            result->data.insert(0, reinterpret_cast<char*>(_data)+header.fileOffset, header.size);
            if (_compressed) {
                result->data = qUncompress(result->data);
            }
        }
    }
    return result;
}

void PointerClip::addFrame(FrameConstPointer) {
    throw std::runtime_error("Pointer clips are read only, use duplicate to create a read/write clip");
}
