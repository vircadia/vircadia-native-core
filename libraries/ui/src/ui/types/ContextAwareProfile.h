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

using ContextAwareProfileParent = QQuickWebEngineProfile;
using RequestInterceptorParent = QWebEngineUrlRequestInterceptor;
#else
#include <QtCore/QObject>

using ContextAwareProfileParent = QObject;
using RequestInterceptorParent = QObject;
#endif

#include <NumericalConstants.h>

class QQmlContext;

class ContextAwareProfile : public ContextAwareProfileParent {
    Q_OBJECT
public:
    static void restrictContext(QQmlContext* context, bool restrict = true);
    bool isRestricted();
    Q_INVOKABLE bool isRestrictedInternal();
protected:

    class RequestInterceptor : public RequestInterceptorParent {
    public:
        RequestInterceptor(ContextAwareProfile* parent) : RequestInterceptorParent(parent), _profile(parent) { }
        bool isRestricted() { return _profile->isRestricted(); }
    protected:
        ContextAwareProfile* _profile;
    };

    ContextAwareProfile(QQmlContext* parent);
    QQmlContext* _context{ nullptr };
    bool _cachedValue{ false };
    quint64 _cacheExpiry{ 0 };
    constexpr static quint64 MAX_CACHE_AGE = MSECS_PER_SECOND;
};

#endif // hifi_FileTypeProfile_h
