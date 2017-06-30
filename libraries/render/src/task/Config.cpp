//
//  Config.cpp
//  render/src/task
//
//  Created by Zach Pomerantz on 1/21/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Config.h"

#include <QtCore/QThread>

#include <shared/QtHelpers.h>

#include "Task.h"

using namespace task;

void JobConfig::setPresetList(const QJsonObject& object) {
    for (auto it = object.begin(); it != object.end(); it++) {
        JobConfig* child = findChild<JobConfig*>(it.key(), Qt::FindDirectChildrenOnly);
        if (child) {
            child->setPresetList(it.value().toObject());
        }
    }
}

void TaskConfig::connectChildConfig(QConfigPointer childConfig, const std::string& name) {
    childConfig->setParent(this);
    childConfig->setObjectName(name.c_str());

    // Connect loaded->refresh
    QObject::connect(childConfig.get(), SIGNAL(loaded()), this, SLOT(refresh()));
    static const char* DIRTY_SIGNAL = "dirty()";
    if (childConfig->metaObject()->indexOfSignal(DIRTY_SIGNAL) != -1) {
        // Connect dirty->refresh if defined
        QObject::connect(childConfig.get(), SIGNAL(dirty()), this, SLOT(refresh()));
        QObject::connect(childConfig.get(), SIGNAL(dirtyEnabled()), this, SLOT(refresh()));
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
            QObject::connect(child, SIGNAL(dirtyEnabled()), this, SLOT(refresh()));
        }
    }
}

void TaskConfig::refresh() {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "refresh");
        return;
    }

    _task->applyConfiguration();
}

