//
//  Task.cpp
//  render/src/render
//
//  Created by Zach Pomerantz on 1/21/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QThread>

#include "Task.h"

using namespace render;

void TaskConfig::refresh() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "refresh", Qt::BlockingQueuedConnection);
        return;
    }

    _task->configure(*this);
}


namespace render{
    
    template <> void varyingGet(const VaryingPairBase* data, uint8_t index, Varying& var) {
        if (index == 0) {
            var = data->first;
        } else {
            var = data->second;
        }
    }
        
    template <> uint8_t varyingLength(const VaryingPairBase* data) { return 2; }
    
}