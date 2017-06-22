//
//  HFTabletWebEngineProfile.h
//  interface/src/networking
//
//  Created by Dante Ruiz on 2017-03-31.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFTabletWebEngineProfile.h"
#include "HFTabletWebEngineRequestInterceptor.h"

static const QString QML_WEB_ENGINE_NAME = "qmlTabletWebEngine";

HFTabletWebEngineProfile::HFTabletWebEngineProfile(QObject* parent) : QQuickWebEngineProfile(parent) {

    static const QString WEB_ENGINE_USER_AGENT = "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36";

    setHttpUserAgent(WEB_ENGINE_USER_AGENT);
    setStorageName(QML_WEB_ENGINE_NAME);

    auto requestInterceptor = new HFTabletWebEngineRequestInterceptor(this);
    setRequestInterceptor(requestInterceptor);
}

