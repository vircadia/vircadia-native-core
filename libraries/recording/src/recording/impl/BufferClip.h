//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Impl_BufferClip_h
#define hifi_Recording_Impl_BufferClip_h

#include "ArrayClip.h"

#include <QtCore/QUuid>

namespace recording {

class BufferClip : public ArrayClip<Frame> {
public:
    using Pointer = std::shared_ptr<BufferClip>;

    virtual QString getName() const override;
    virtual void addFrame(FrameConstPointer) override;

private:
    virtual FrameConstPointer readFrame(size_t index) const override;
    QString _name { QUuid().toString() };
    
    //mutable size_t _frameIndex { 0 }; // FIXME - not in use
};

}

#endif
