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
    TMonitorMap::iterator lookup = gl_monitorMap.find(_context);
    if (lookup != gl_monitorMap.end()) {
        gl_monitorMap.erase(lookup);
    }
    gl_monitorMapProtect.unlock();
}

RestrictedContextMonitor::TSharedPointer RestrictedContextMonitor::getMonitor(QQmlContext* context, bool createIfMissing) {
    TSharedPointer monitor;

    gl_monitorMapProtect.lock();
    TMonitorMap::const_iterator lookup = gl_monitorMap.find(context);
    if (lookup != gl_monitorMap.end()) {
        monitor = lookup.value().lock();
        assert(monitor);
    } else if(createIfMissing) {
        monitor = TSharedPointer::create(context);
        monitor->_selfPointer = monitor;
        gl_monitorMap.insert(context, monitor);
    }
    gl_monitorMapProtect.unlock();
    return monitor;
}

ContextAwareProfile::ContextAwareProfile(QQmlContext* context) : ContextAwareProfileParent(context), _context(context) {
    assert(context);

    _monitor = RestrictedContextMonitor::getMonitor(context, true);
    assert(_monitor);
    connect(_monitor.get(), &RestrictedContextMonitor::onIsRestrictedChanged, this, &ContextAwareProfile::onIsRestrictedChanged);
    if (_monitor->_isUninitialized) {
        _monitor->_isRestricted = isRestrictedGetProperty();
        _monitor->_isUninitialized = false;
    }
    _isRestricted.store(_monitor->_isRestricted ? 1 : 0);
}

void ContextAwareProfile::restrictContext(QQmlContext* context, bool restrict) {
    RestrictedContextMonitor::TSharedPointer monitor = RestrictedContextMonitor::getMonitor(context, false);

    context->setContextProperty(RESTRICTED_FLAG_PROPERTY, restrict);
    if (monitor && monitor->_isRestricted != restrict) {
        monitor->_isRestricted = restrict;
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
