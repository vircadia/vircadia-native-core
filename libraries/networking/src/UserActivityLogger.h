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

#include "AccountManager.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkReply>

class UserActivityLogger : public QObject {
    Q_OBJECT
    
public:
    static UserActivityLogger& getInstance();
    
public slots:
    void disable(bool disable);
    void logAction(QString action, QJsonObject details = QJsonObject(), JSONCallbackParameters params = JSONCallbackParameters());
    
    void launch(QString applicationVersion);
    void close(int delayTime);
    void changedDisplayName(QString displayName);
    void changedModel(QString typeOfModel, QString modelURL);
    void changedDomain(QString domainURL);
    void connectedDevice(QString typeOfDevice, QString deviceName);
    void loadedScript(QString scriptName);
    void wentTo(QString destinationType, QString destinationName);
    
private slots:
    void requestFinished(const QJsonObject& object);
    void requestError(QNetworkReply::NetworkError error,const QString& string);
    
private:
    UserActivityLogger();
    bool _disabled;
};

#endif // hifi_UserActivityLogger_h