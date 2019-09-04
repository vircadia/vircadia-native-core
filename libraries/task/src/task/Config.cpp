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

JobConfig::~JobConfig() {
    
}

void JobConfig::setEnabled(bool enable) {
    if (_isEnabled != enable) {
        _isEnabled = enable;
        emit dirtyEnabled();
    }
}

void JobConfig::setPresetList(const QJsonObject& object) {
    for (auto it = object.begin(); it != object.end(); it++) {
        JobConfig* child = findChild<JobConfig*>(it.key(), Qt::FindDirectChildrenOnly);
        if (child) {
            child->setPresetList(it.value().toObject());
        }
    }
}

void JobConfig::connectChildConfig(std::shared_ptr<JobConfig> childConfig, const std::string& name) {
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

void JobConfig::transferChildrenConfigs(std::shared_ptr<JobConfig> source) {
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

void JobConfig::refresh() {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "refresh");
        return;
    }

    _jobConcept->applyConfiguration();
}

JobConfig* JobConfig::getRootConfig(const std::string& jobPath, std::string& jobName) const {
    JobConfig* root = const_cast<JobConfig*> (this);

    std::list<std::string> tokens;
    std::size_t pos = 0, sepPos;
    while ((sepPos = jobPath.find_first_of('.', pos)) != std::string::npos) {
        std::string token = jobPath.substr(pos, sepPos - pos);
        if (!token.empty()) {
            tokens.push_back(token);
        }
        pos = sepPos + 1;
    }
    {
        std::string token = jobPath.substr(pos, sepPos - pos);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    if (tokens.empty()) {
        return root;
    }
    else {
        while (tokens.size() > 1) {
            auto taskName = tokens.front();
            tokens.pop_front();
            root = root->findChild<JobConfig*>((taskName.empty() ? QString() : QString(taskName.c_str())));
            if (!root) {
                return nullptr;
            }
        }
        jobName = tokens.front();
    }
    return root;
}

JobConfig* JobConfig::getJobConfig(const std::string& jobPath) const {
    std::string jobName;
    auto root = getRootConfig(jobPath, jobName);

    if (!root) {
        return nullptr;
    } 
    if (jobName.empty()) {
        return root;
    } else {

        auto found = root->findChild<JobConfig*>((jobName.empty() ? QString() : QString(jobName.c_str())));
        if (!found) {
            return nullptr;
        }
        return found;
    }
}

void JobConfig::setBranch(uint8_t branch) {
    if (_branch != branch) {
        _branch = branch;
        // We can re-use this signal here
        emit dirtyEnabled();
    }
}
