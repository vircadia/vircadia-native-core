//
//  NetworkLogging.h
//  libraries/networking/src
//
//  Created by Seth Alves on 4/6/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkLogging_h
#define hifi_NetworkLogging_h

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(resourceLog)
Q_DECLARE_LOGGING_CATEGORY(networking)
Q_DECLARE_LOGGING_CATEGORY(asset_client)
Q_DECLARE_LOGGING_CATEGORY(entity_script_client)
Q_DECLARE_LOGGING_CATEGORY(messages_client)

#define HIFI_FDEBUG(category, msg) \
    do { \
        if (category().isDebugEnabled()) { \
            static int repeatedMessageID_ = LogHandler::getInstance().newRepeatedMessageID(); \
            QString logString_; \
            QDebug debugString_(&logString_); \
            debugString_ << msg; \
            LogHandler::getInstance().printRepeatedMessage(repeatedMessageID_, LogDebug, QMessageLogContext(), logString_); \
        } \
    } while (false)

#endif // hifi_NetworkLogging_h
