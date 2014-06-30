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

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkReply>

class UserActivityLogger : public QObject {
    Q_OBJECT
    
public:
    static UserActivityLogger& getInstance();
    
public slots:
    void logAction(QString action, QJsonObject details = QJsonObject());
    
    void login();
    void logout();
//    void changedDisplayName();
    void changedModel();
    void changedDomain();
//    void connectedDevice();
    
private slots:
    void requestFinished(const QJsonObject& object);
    void requestError(QNetworkReply::NetworkError error,const QString& string);
    
private:
    UserActivityLogger();
};

#endif // hifi_UserActivityLogger_h