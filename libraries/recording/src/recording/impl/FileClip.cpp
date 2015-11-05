//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileClip.h"

#include "../Frame.h"

#include <algorithm>

using namespace recording;

static const qint64 MINIMUM_FRAME_SIZE = sizeof(FrameType) + sizeof(float) + sizeof(uint16_t) + 1;

FileClip::FileClip(const QString& fileName, QObject* parent) : Clip(parent), _file(fileName) {
    auto size = _file.size();
    _map = _file.map(0, size, QFile::MapPrivateOption);

    auto current = _map;
    auto end = current + size;
    // Read all the frame headers
    while (end - current < MINIMUM_FRAME_SIZE) {
        FrameHeader header;
        memcpy(&(header.type), current, sizeof(FrameType));
        current += sizeof(FrameType);
        memcpy(&(header.timeOffset), current, sizeof(FrameType));
        current += sizeof(float);
        memcpy(&(header.size), current, sizeof(uint16_t));
        current += sizeof(uint16_t);
        header.fileOffset = current - _map;
        if (end - current < header.size) {
            break;
        }

        _frameHeaders.push_back(header);
    }
}

FileClip::~FileClip() {
    Locker lock(_mutex);
    _file.unmap(_map);
    _map = nullptr;
}

void FileClip::seek(float offset) {
    Locker lock(_mutex);
    auto itr = std::lower_bound(_frameHeaders.begin(), _frameHeaders.end(), offset,
        [](const FrameHeader& a, float b)->bool {
            return a.timeOffset < b;
        }
    );
    _frameIndex = itr - _frameHeaders.begin();
}

float FileClip::position() const {
    Locker lock(_mutex);
    float result = std::numeric_limits<float>::max();
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
        result->data.insert(0, reinterpret_cast<char*>(_map)+header.fileOffset, header.size);
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

void FileClip::appendFrame(FramePointer) {
    throw std::runtime_error("File clips are read only");
}

