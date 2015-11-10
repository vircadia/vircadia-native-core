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
    auto result = std::make_shared<FileClip>(filePath);
    if (result->frameCount() == 0) {
        return Clip::Pointer();
    }
    return result;
}

void Clip::toFile(const QString& filePath, Clip::Pointer clip) {
    FileClip::write(filePath, clip->duplicate());
}

Clip::Pointer Clip::newClip() {
    return std::make_shared<BufferClip>();
}

Clip::Pointer Clip::duplicate() {
    Clip::Pointer result = std::make_shared<BufferClip>();

    Locker lock(_mutex);
    float currentPosition = position();
    seek(0);

    Frame::Pointer frame = nextFrame();
    while (frame) {
        result->addFrame(frame);
        frame = nextFrame();
    }

    seek(currentPosition);
    return result;
}
