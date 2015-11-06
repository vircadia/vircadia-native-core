//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Impl_FileClip_h
#define hifi_Recording_Impl_FileClip_h

#include "../Clip.h"

#include <QtCore/QFile>

#include <mutex>

namespace recording {

class FileClip : public Clip {
public:
    using Pointer = std::shared_ptr<FileClip>;

    FileClip(const QString& file, QObject* parent = nullptr);
    virtual ~FileClip();

    virtual void seek(float offset) override;
    virtual float position() const override;

    virtual FramePointer peekFrame() const override;
    virtual FramePointer nextFrame() override;
    virtual void appendFrame(FramePointer) override;
    virtual void skipFrame() override;

private:
    using Mutex = std::mutex;
    using Locker = std::unique_lock<Mutex>;

    virtual void reset() override;

    struct FrameHeader {
        FrameType type;
        float timeOffset;
        uint16_t size;
        quint64 fileOffset;
    };

    using FrameHeaders = std::vector<FrameHeader>;

    FramePointer readFrame(uint32_t frameIndex) const;

    mutable Mutex _mutex;
    QFile _file;
    uint32_t _frameIndex { 0 };
    uchar* _map;
    FrameHeaders _frameHeaders;
};

}

#endif
