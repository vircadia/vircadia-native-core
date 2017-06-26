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

#include "FileTypeRequestInterceptor.h"

static const QString QML_WEB_ENGINE_STORAGE_NAME = "qmlWebEngine";

FileTypeProfile::FileTypeProfile(QObject* parent) :
    QQuickWebEngineProfile(parent)
{
    static const QString WEB_ENGINE_USER_AGENT = "Chrome/48.0 (HighFidelityInterface)";
    setHttpUserAgent(WEB_ENGINE_USER_AGENT);

    auto requestInterceptor = new FileTypeRequestInterceptor(this);
    setRequestInterceptor(requestInterceptor);
}
