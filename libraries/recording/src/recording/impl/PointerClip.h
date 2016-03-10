//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Impl_PointerClip_h
#define hifi_Recording_Impl_PointerClip_h

#include "ArrayClip.h"

#include <mutex>

#include <QtCore/QJsonDocument>

#include "../Frame.h"

namespace recording {

struct PointerFrameHeader : public FrameHeader {
    FrameType type;
    Frame::Time timeOffset;
    uint16_t size;
    quint64 fileOffset;
};

using PointerFrameHeaderList = std::list<PointerFrameHeader>;

class PointerClip : public ArrayClip<PointerFrameHeader> {
public:
    using Pointer = std::shared_ptr<PointerClip>;

    PointerClip() {};
    PointerClip(uchar* data, size_t size) { init(data, size); }

    void init(uchar* data, size_t size);
    virtual void addFrame(FrameConstPointer) override;
    const QJsonDocument& getHeader() const {
        return _header;
    }

    // FIXME move to frame?
    static const qint64 MINIMUM_FRAME_SIZE = sizeof(FrameType) + sizeof(Frame::Time) + sizeof(FrameSize);
protected:
    void reset() override;
    virtual FrameConstPointer readFrame(size_t index) const override;
    QJsonDocument _header;
    uchar* _data { nullptr };
    size_t _size { 0 };
    bool _compressed { true };
};

}

#endif
