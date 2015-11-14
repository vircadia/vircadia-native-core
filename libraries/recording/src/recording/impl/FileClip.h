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

#include "ArrayClip.h"

#include <mutex>

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>

#include "../Frame.h"

namespace recording {

struct FileFrameHeader : public FrameHeader {
    FrameType type;
    Frame::Time timeOffset;
    uint16_t size;
    quint64 fileOffset;
};

using FileFrameHeaderList = std::list<FileFrameHeader>;

class FileClip : public ArrayClip<FileFrameHeader> {
public:
    using Pointer = std::shared_ptr<FileClip>;

    FileClip(const QString& file);
    virtual ~FileClip();

    virtual QString getName() const override;
    virtual void addFrame(FrameConstPointer) override;

    const QJsonDocument& getHeader() {
        return _fileHeader;
    }

    static bool write(const QString& filePath, Clip::Pointer clip);

private:
    virtual FrameConstPointer readFrame(size_t index) const override;
    QJsonDocument _fileHeader;
    QFile _file;
    uchar* _map { nullptr };
    bool _compressed { true };
};

}

#endif
