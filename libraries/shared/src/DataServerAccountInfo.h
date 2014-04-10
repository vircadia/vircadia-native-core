//
//  DataServerAccountInfo.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DataServerAccountInfo_h
#define hifi_DataServerAccountInfo_h

#include <QtCore/QObject>

#include "OAuthAccessToken.h"

class DataServerAccountInfo : public QObject {
    Q_OBJECT
public:
    DataServerAccountInfo();
    DataServerAccountInfo(const QJsonObject& jsonObject);
    DataServerAccountInfo(const DataServerAccountInfo& otherInfo);
    DataServerAccountInfo& operator=(const DataServerAccountInfo& otherInfo);
    
    const OAuthAccessToken& getAccessToken() const { return _accessToken; }
    
    const QString& getUsername() const { return _username; }
    void setUsername(const QString& username);

    const QString& getXMPPPassword() const { return _xmppPassword; }
    void setXMPPPassword(const QString& xmppPassword);
    
    friend QDataStream& operator<<(QDataStream &out, const DataServerAccountInfo& info);
    friend QDataStream& operator>>(QDataStream &in, DataServerAccountInfo& info);
private:
    void swap(DataServerAccountInfo& otherInfo);
    
    OAuthAccessToken _accessToken;
    QString _username;
    QString _xmppPassword;
};

#endif // hifi_DataServerAccountInfo_h
