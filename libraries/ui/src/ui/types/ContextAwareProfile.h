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
#include <QtCore/QSharedPointer>

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
    typedef QSharedPointer<RestrictedContextMonitor> TSharedPointer;
    typedef QWeakPointer<RestrictedContextMonitor> TWeakPointer;

    inline RestrictedContextMonitor(QQmlContext* c) : _context(c) {}
    ~RestrictedContextMonitor();

    static TSharedPointer getMonitor(QQmlContext* context, bool createIfMissing);

signals:
    void onIsRestrictedChanged(bool newValue);

public:
    TWeakPointer _selfPointer;
    QQmlContext* _context{ nullptr };
    bool _isRestricted{ true };
    bool _isUninitialized{ true };

private:
    typedef std::map<QQmlContext*, TWeakPointer> TMonitorMap;

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
    RestrictedContextMonitor::TSharedPointer _monitor;
};

#endif // hifi_FileTypeProfile_h
