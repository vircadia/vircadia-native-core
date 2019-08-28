//
//  Created by Bradley Austin Davis on 2018/07/27
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_ContextAwareProfile_h
#define hifi_ContextAwareProfile_h

#include <QtCore/QtGlobal>

#if !defined(Q_OS_ANDROID)
#include <QtWebEngine/QQuickWebEngineProfile>
#include <QtWebEngineCore/QWebEngineUrlRequestInterceptor>

class QQmlContext;

class ContextAwareProfile : public QQuickWebEngineProfile {
    Q_OBJECT
public:
    static void restrictContext(QQmlContext* context, bool restrict = true);
    Q_INVOKABLE bool isRestricted();
protected:

    class RequestInterceptor : public QWebEngineUrlRequestInterceptor {
    public:
        RequestInterceptor(ContextAwareProfile* parent) : QWebEngineUrlRequestInterceptor(parent), _profile(parent) {}
        bool isRestricted() { return _profile->isRestricted(); }
    protected:
        ContextAwareProfile* _profile;
    };

    ContextAwareProfile(QQmlContext* parent);
    QQmlContext* _context;
};
#endif

#endif // hifi_FileTypeProfile_h
