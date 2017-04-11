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

void TaskConfig::connectChildConfig(QConfigPointer childConfig, const std::string& name) {
    childConfig->setParent(this);
    childConfig->setObjectName(name.c_str());

    // Connect loaded->refresh
    QObject::connect(childConfig.get(), SIGNAL(loaded()), this, SLOT(refresh()));
    static const char* DIRTY_SIGNAL = "dirty()";
    if (childConfig->metaObject()->indexOfSignal(DIRTY_SIGNAL) != -1) {
        // Connect dirty->refresh if defined
        QObject::connect(childConfig.get(), SIGNAL(dirty()), this, SLOT(refresh()));
    }
}

void TaskConfig::transferChildrenConfigs(QConfigPointer source) {
    if (!source) {
        return;
    }
    // Transfer children to the new configuration
    auto children = source->children();
    for (auto& child : children) {
        child->setParent(this);
        QObject::connect(child, SIGNAL(loaded()), this, SLOT(refresh()));
        static const char* DIRTY_SIGNAL = "dirty()";
        if (child->metaObject()->indexOfSignal(DIRTY_SIGNAL) != -1) {
            // Connect dirty->refresh if defined
            QObject::connect(child, SIGNAL(dirty()), this, SLOT(refresh()));
        }
    }
}

void TaskConfig::refresh() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "refresh", Qt::BlockingQueuedConnection);
        return;
    }

    _task->applyConfiguration();
}

