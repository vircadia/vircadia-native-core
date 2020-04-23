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
#include <QtCore/QMutex>

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

class RestrictedContextMonitor : public QObject {
    Q_OBJECT
public:
    typedef std::shared_ptr<RestrictedContextMonitor> TSharedPtr;
    typedef std::weak_ptr<RestrictedContextMonitor> TWeakPtr;

    inline RestrictedContextMonitor(QQmlContext* c) : context(c) {}
    ~RestrictedContextMonitor();

    static TSharedPtr getMonitor(QQmlContext* context, bool createIfMissing);

signals:
    void onIsRestrictedChanged(bool newValue);

public:
    TWeakPtr selfPtr;
    QQmlContext* context{ nullptr };
    bool isRestricted{ true };
    bool isUninitialized{ true };

private:
    typedef std::map<QQmlContext*, TWeakPtr> TMonitorMap;

    static QMutex gl_monitorMapProtect;
    static TMonitorMap gl_monitorMap;
};

class ContextAwareProfile : public ContextAwareProfileParent {
    Q_OBJECT
public:
    static void restrictContext(QQmlContext* context, bool restrict = true);
    bool isRestricted();
    Q_INVOKABLE bool isRestrictedGetProperty();
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
    QAtomicInt _isRestricted{ 0 };

private slots:
    void onIsRestrictedChanged(bool newValue);

private:
    RestrictedContextMonitor::TSharedPtr _monitor;
};

#endif // hifi_FileTypeProfile_h
