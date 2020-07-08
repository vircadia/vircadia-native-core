//
//  FileTypeProfile.h
//  interface/src/networking
//
//  Created by Kunal Gosar on 2017-03-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_FileTypeProfile_h
#define hifi_FileTypeProfile_h

#include <QtCore/QtGlobal>

#if !defined(Q_OS_ANDROID)
#include "ContextAwareProfile.h"

class FileTypeProfile : public ContextAwareProfile {
    using Parent = ContextAwareProfile;

public:
    static void registerWithContext(QQmlContext* parent);

    static void clearCache();

protected:
    FileTypeProfile(QQmlContext* parent);
    virtual ~FileTypeProfile();

    class RequestInterceptor : public Parent::RequestInterceptor {
    public:
        RequestInterceptor(ContextAwareProfile* parent) : Parent::RequestInterceptor(parent) {}
        void interceptRequest(QWebEngineUrlRequestInfo& info) override;
    };
};

#endif

#endif // hifi_FileTypeProfile_h
