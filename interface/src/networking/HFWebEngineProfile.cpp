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

#include "HFWebEngineRequestInterceptor.h"

static const QString QML_WEB_ENGINE_STORAGE_NAME = "qmlWebEngine";

HFWebEngineProfile::HFWebEngineProfile(QObject* parent) :
    QQuickWebEngineProfile(parent)
{
    static const QString WEB_ENGINE_USER_AGENT = "Chrome/48.0 (HighFidelityInterface)";
    setHttpUserAgent(WEB_ENGINE_USER_AGENT);

    // we use the HFWebEngineRequestInterceptor to make sure that web requests are authenticated for the interface user
    auto requestInterceptor = new HFWebEngineRequestInterceptor(this);
    setRequestInterceptor(requestInterceptor);
}
