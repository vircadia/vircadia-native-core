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
#include <QtCore/QThread>
#include <shared/QtHelpers.h>

#if !defined(Q_OS_ANDROID)

#include <QtQml/QQmlContext>

static const QString RESTRICTED_FLAG_PROPERTY = "RestrictFileAccess";

ContextAwareProfile::ContextAwareProfile(QQmlContext* context) :
    QQuickWebEngineProfile(context), _context(context) { }


void ContextAwareProfile::restrictContext(QQmlContext* context, bool restrict) {
    context->setContextProperty(RESTRICTED_FLAG_PROPERTY, restrict);
}

bool ContextAwareProfile::isRestricted() {
    if (QThread::currentThread() != thread()) {
        bool restrictedResult = false;
        BLOCKING_INVOKE_METHOD(this, "isRestricted", Q_RETURN_ARG(bool, restrictedResult));
    }
    return _context->contextProperty(RESTRICTED_FLAG_PROPERTY).toBool();
}

#endif