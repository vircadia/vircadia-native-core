//
//  FileTypeProfile.cpp
//  interface/src/networking
//
//  Created by Kunal Gosar on 2017-03-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileTypeProfile.h"

#include <set>
#include <mutex>
#include <QtQml/QQmlContext>

#include "RequestFilters.h"
#include "NetworkingConstants.h"

#if !defined(Q_OS_ANDROID)
static const QString QML_WEB_ENGINE_STORAGE_NAME = "qmlWebEngine";

static std::set<FileTypeProfile*> FileTypeProfile_instances;
static std::mutex FileTypeProfile_mutex;

FileTypeProfile::FileTypeProfile(QQmlContext* parent) :
    ContextAwareProfile(parent)
{
    setHttpUserAgent(NetworkingConstants::WEB_ENGINE_USER_AGENT);

    setStorageName(QML_WEB_ENGINE_STORAGE_NAME);
    setOffTheRecord(false);

    auto requestInterceptor = new RequestInterceptor(this);
    setRequestInterceptor(requestInterceptor);

    std::lock_guard<std::mutex> lock(FileTypeProfile_mutex);
    FileTypeProfile_instances.insert(this);
}

FileTypeProfile::~FileTypeProfile() {
    std::lock_guard<std::mutex> lock(FileTypeProfile_mutex);
    FileTypeProfile_instances.erase(this);
}

void FileTypeProfile::RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    RequestFilters::interceptHFWebEngineRequest(info, isRestricted());
    RequestFilters::interceptFileType(info);
}

void FileTypeProfile::registerWithContext(QQmlContext* context) {
    context->setContextProperty("FileTypeProfile", new FileTypeProfile(context));
}

void FileTypeProfile::clearCache() {
    std::lock_guard<std::mutex> lock(FileTypeProfile_mutex);
    foreach (auto instance, FileTypeProfile_instances) {
        instance->clearHttpCache();
    }
}

#endif