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
#include <QtCore/QReadLocker>
#include <QtCore/QThread>
#include <QtCore/QWriteLocker>
#include <QtQml/QQmlContext>

#include <shared/QtHelpers.h>
#include <SharedUtil.h>

static const QString RESTRICTED_FLAG_PROPERTY = "RestrictFileAccess";

QReadWriteLock ContextAwareProfile::gl_contextMapProtect;
ContextAwareProfile::ContextMap ContextAwareProfile::gl_contextMap;

ContextAwareProfile::ContextAwareProfile(QQmlContext* context) : ContextAwareProfileParent(context), _context(context) {
    assert(context);

    {   // register our object for future updates
        QWriteLocker guard(&gl_contextMapProtect);
        ContextMap::iterator setLookup = gl_contextMap.find(_context);
        if (setLookup == gl_contextMap.end()) {
            setLookup = gl_contextMap.insert(_context, ContextAwareProfileSet());
        }
        assert(setLookup != gl_contextMap.end());
        ContextAwareProfileSet& profileSet = setLookup.value();
        assert(profileSet.find(this) == profileSet.end());
        profileSet.insert(this);
    }

    _isRestricted.store(isRestrictedGetProperty());
}

ContextAwareProfile::~ContextAwareProfile() {
    {  // deregister our object
        QWriteLocker guard(&gl_contextMapProtect);
        ContextMap::iterator setLookup = gl_contextMap.find(_context);
        assert(setLookup != gl_contextMap.end());
        if (setLookup != gl_contextMap.end()) {
            ContextAwareProfileSet& profileSet = setLookup.value();
            assert(profileSet.find(this) != profileSet.end());
            profileSet.remove(this);
            if (profileSet.isEmpty()) {
                gl_contextMap.erase(setLookup);
            }
        }
    }
}

void ContextAwareProfile::restrictContext(QQmlContext* context, bool restrict) {

    // set the QML property
    context->setContextProperty(RESTRICTED_FLAG_PROPERTY, restrict);

    // broadcast the new value to any registered ContextAwareProfile objects
    {  // deregister our object
        QReadLocker guard(&gl_contextMapProtect);
        ContextMap::const_iterator setLookup = gl_contextMap.find(context);
        if (setLookup != gl_contextMap.end()) {
            const ContextAwareProfileSet& profileSet = setLookup.value();
            for (ContextAwareProfileSet::const_iterator profileIterator = profileSet.begin();
                 profileIterator != profileSet.end(); profileIterator++) {
                (*profileIterator)->onIsRestrictedChanged(restrict);
            }
        }
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
    _isRestricted.store(newValue);
}

bool ContextAwareProfile::isRestricted() {
    return _isRestricted.load();
}
