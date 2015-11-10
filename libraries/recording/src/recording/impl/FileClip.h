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
#include <QtCore/QJsonDocument>

#include <mutex>

namespace recording {

class FileClip : public Clip {
public:
    using Pointer = std::shared_ptr<FileClip>;

    FileClip(const QString& file);
    virtual ~FileClip();

    virtual Time duration() const override;
    virtual size_t frameCount() const override;

    virtual void seek(Time offset) override;
    virtual Time position() const override;

    virtual FramePointer peekFrame() const override;
    virtual FramePointer nextFrame() override;
    virtual void skipFrame() override;
    virtual void addFrame(FramePointer) override;

    const QJsonDocument& getHeader() {
        return _fileHeader;
    }

    static bool write(const QString& filePath, Clip::Pointer clip);

    struct FrameHeader {
        FrameType type;
        Time timeOffset;
        uint16_t size;
        quint64 fileOffset;
    };

private:

    virtual void reset() override;


    using FrameHeaderVector = std::vector<FrameHeader>;

    FramePointer readFrame(uint32_t frameIndex) const;

    QJsonDocument _fileHeader;
    QFile _file;
    uint32_t _frameIndex { 0 };
    uchar* _map { nullptr };
    FrameHeaderVector _frameHeaders;
};

}

#endif
