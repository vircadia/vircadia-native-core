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

#if !defined(Q_OS_ANDROID)

#include <QtQml/QQmlContext>

static const QString RESTRICTED_FLAG_PROPERTY = "RestrictFileAccess";

ContextAwareProfile::ContextAwareProfile(QQmlContext* parent) :
    QQuickWebEngineProfile(parent), _context(parent) { }


void ContextAwareProfile::restrictContext(QQmlContext* context) {
    context->setContextProperty(RESTRICTED_FLAG_PROPERTY, true);
}

bool ContextAwareProfile::isRestricted(QQmlContext* context) {
    return context->contextProperty(RESTRICTED_FLAG_PROPERTY).toBool();
}

#endif