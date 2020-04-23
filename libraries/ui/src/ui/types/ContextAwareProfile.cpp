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

#include "ContextAwareProfile.h"

#include <cassert>
#include <QtCore/QThread>
#include <QtQml/QQmlContext>

#include <shared/QtHelpers.h>
#include <SharedUtil.h>

static const QString RESTRICTED_FLAG_PROPERTY = "RestrictFileAccess";

QMutex RestrictedContextMonitor::gl_monitorMapProtect;
RestrictedContextMonitor::TMonitorMap RestrictedContextMonitor::gl_monitorMap;

RestrictedContextMonitor::~RestrictedContextMonitor() {
    gl_monitorMapProtect.lock();
    TMonitorMap::iterator lookup = gl_monitorMap.find(context);
    if (lookup != gl_monitorMap.end()) {
        gl_monitorMap.erase(lookup);
    }
    gl_monitorMapProtect.unlock();
}

RestrictedContextMonitor::TSharedPtr RestrictedContextMonitor::getMonitor(QQmlContext* context, bool createIfMissing) {
    TSharedPtr monitor;

    gl_monitorMapProtect.lock();
    TMonitorMap::const_iterator lookup = gl_monitorMap.find(context);
    if (lookup != gl_monitorMap.end()) {
        monitor = lookup->second.lock();
        assert(monitor);
    } else if(createIfMissing) {
        monitor = std::make_shared<RestrictedContextMonitor>(context);
        monitor->selfPtr = monitor;
        gl_monitorMap.insert(TMonitorMap::value_type(context, monitor));
    }
    gl_monitorMapProtect.unlock();
    return monitor;
}

ContextAwareProfile::ContextAwareProfile(QQmlContext* context) : ContextAwareProfileParent(context), _context(context) {
    assert(context);

    _monitor = RestrictedContextMonitor::getMonitor(context, true);
    assert(_monitor);
    connect(_monitor.get(), &RestrictedContextMonitor::onIsRestrictedChanged, this, &ContextAwareProfile::onIsRestrictedChanged);
    if (_monitor->isUninitialized) {
        _monitor->isRestricted = isRestrictedGetProperty();
        _monitor->isUninitialized = false;
    }
    _isRestricted.store(_monitor->isRestricted ? 1 : 0);
}

void ContextAwareProfile::restrictContext(QQmlContext* context, bool restrict) {
    RestrictedContextMonitor::TSharedPtr monitor = RestrictedContextMonitor::getMonitor(context, false);

    context->setContextProperty(RESTRICTED_FLAG_PROPERTY, restrict);
    if (monitor && monitor->isRestricted != restrict) {
        monitor->isRestricted = restrict;
        monitor->onIsRestrictedChanged(restrict);
    }
}

bool ContextAwareProfile::isRestrictedGetProperty() {
    if (QThread::currentThread() != thread()) {
        bool restrictedResult = false;
        BLOCKING_INVOKE_METHOD(this, "isRestrictedGetProperty", Q_RETURN_ARG(bool, restrictedResult));
        return restrictedResult;
    }

    QVariant variant = _context->contextProperty(RESTRICTED_FLAG_PROPERTY);
    if (variant.isValid()) {
        return variant.toBool();
    }

    // BUGZ-1365 - we MUST defalut to restricted mode in the absence of a flag, or it's too easy for someone to make 
    // a new mechanism for loading web content that fails to restrict access to local files
    return true;
}

void ContextAwareProfile::onIsRestrictedChanged(bool newValue) {
    _isRestricted.store(newValue ? 1 : 0);
}

bool ContextAwareProfile::isRestricted() {
    return _isRestricted.load() != 0;
}
