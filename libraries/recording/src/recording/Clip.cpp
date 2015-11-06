//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Clip.h"

#include "Frame.h"

#include "impl/FileClip.h"
#include "impl/BufferClip.h"

using namespace recording;

Clip::Pointer Clip::fromFile(const QString& filePath) {
    return std::make_shared<FileClip>(filePath);
}

void Clip::toFile(Clip::Pointer clip, const QString& filePath) {
    // FIXME
}

Clip::Pointer Clip::duplicate() {
    Clip::Pointer result = std::make_shared<BufferClip>();

    float currentPosition = position();
    seek(0);

    Frame::Pointer frame = nextFrame();
    while (frame) {
        result->appendFrame(frame);
    }

    seek(currentPosition);
    return result;
}

#if 0
Clip::Pointer Clip::fromIODevice(QIODevice * device) {
    return std::make_shared<IOClip>(device);
}

void Clip::fromIODevice(Clip::Pointer clip, QIODevice * device) {
}
#endif