//
//  HFWebEngineProfile.cpp
//  interface/src/networking
//
//  Created by Stephen Birarda on 2016-10-17.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFWebEngineProfile.h"

#include <set>
#include <mutex>
#include <QtQml/QQmlContext>

#include "RequestFilters.h"

#if !defined(Q_OS_ANDROID)

static const QString QML_WEB_ENGINE_STORAGE_NAME = "qmlWebEngine";

static std::set<HFWebEngineProfile*> HFWebEngineProfile_instances;
static std::mutex HFWebEngineProfile_mutex;

HFWebEngineProfile::HFWebEngineProfile(QQmlContext* parent) : Parent(parent)
{
    setStorageName(QML_WEB_ENGINE_STORAGE_NAME);
    setOffTheRecord(false);

    // we use the HFWebEngineRequestInterceptor to make sure that web requests are authenticated for the interface user
    setRequestInterceptor(new RequestInterceptor(this));

    std::lock_guard<std::mutex> lock(HFWebEngineProfile_mutex);
    HFWebEngineProfile_instances.insert(this);
}

HFWebEngineProfile::~HFWebEngineProfile() {
    std::lock_guard<std::mutex> lock(HFWebEngineProfile_mutex);
    HFWebEngineProfile_instances.erase(this);
}

void HFWebEngineProfile::RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    RequestFilters::interceptHFWebEngineRequest(info, isRestricted());
}

void HFWebEngineProfile::registerWithContext(QQmlContext* context) {
    context->setContextProperty("HFWebEngineProfile", new HFWebEngineProfile(context));
}

void HFWebEngineProfile::clearCache() {
    std::lock_guard<std::mutex> lock(HFWebEngineProfile_mutex);
    foreach (auto instance, HFWebEngineProfile_instances) {
        instance->clearHttpCache();
    }
}

#endif
