//
//  VerboseLoggingHelper.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-10-28.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qdebug.h>

#include "VerboseLoggingHelper.h"

VerboseLoggingHelper& VerboseLoggingHelper::getInstance() {
    static VerboseLoggingHelper sharedInstance;
    return sharedInstance;
}

VerboseLoggingHelper::VerboseLoggingHelper() {
    // setup our timer to flush the verbose logs every 5 seconds
    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &VerboseLoggingHelper::flushMessages);
    _timer->start(VERBOSE_LOG_INTERVAL_SECONDS * 1000);
}

void VerboseLoggingHelper::flushMessages() {
    QHash<QString, int>::iterator message = _messageCountHash.begin();
    while (message != _messageCountHash.end()) {
        qDebug() << message.key().arg(message.value());
        message = _messageCountHash.erase(message);
    }
}