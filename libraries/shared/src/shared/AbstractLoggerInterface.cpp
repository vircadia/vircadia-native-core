//
//  Created by Bradley Austin Davis on 2016/11/29
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AbstractLoggerInterface.h"
#include "GlobalAppProperties.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>

AbstractLoggerInterface* AbstractLoggerInterface::get() {
    QVariant loggerVar = qApp->property(hifi::properties::LOGGER);
    QObject* loggerObject = qvariant_cast<QObject *>(loggerVar);
    return qobject_cast<AbstractLoggerInterface*>(loggerObject);
}

AbstractLoggerInterface::AbstractLoggerInterface(QObject* parent) : QObject(parent) {
    qApp->setProperty(hifi::properties::LOGGER, QVariant::fromValue(this));
}

AbstractLoggerInterface::~AbstractLoggerInterface() {
    if (qApp) {
        qApp->setProperty(hifi::properties::LOGGER, QVariant());
    }
}

