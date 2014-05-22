//
//  UserActivityLogger.h
//
//
//  Created by Clement on 5/21/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UserActivityLogger_h
#define hifi_UserActivityLogger_h

#include <QString>
#include <QJsonObject>

class UserActivityLogger {
public:
    static UserActivityLogger& getInstance();
    
    void login();
    void logout();
    
private:
    UserActivityLogger();
    void logAction(QString action, QJsonObject details = QJsonObject());
};

#endif // hifi_UserActivityLogger_h