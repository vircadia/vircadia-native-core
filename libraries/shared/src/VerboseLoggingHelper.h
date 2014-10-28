//
//  VerboseLoggingHelper.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-10-28.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VerboseLoggingHelper_h
#define hifi_VerboseLoggingHelper_h

#include <qhash.h>
#include <qobject.h>
#include <qtimer.h>

const int VERBOSE_LOG_INTERVAL_SECONDS = 5;

class VerboseLoggingHelper : public QObject {
    Q_OBJECT
public:
    static VerboseLoggingHelper& getInstance();
    
    void addMessage(const QString& message) { _messageCountHash[message] += 1; }
private:
    VerboseLoggingHelper();
    
    void flushMessages();
    
    QTimer* _timer;
    QHash<QString, int> _messageCountHash;
};

#endif // hifi_VerboseLoggingHelper_h