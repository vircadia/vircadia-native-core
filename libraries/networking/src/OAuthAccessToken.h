//
//  OAuthAccessToken.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OAuthAccessToken_h
#define hifi_OAuthAccessToken_h

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>

class OAuthAccessToken : public QObject {
    Q_OBJECT
public:
    OAuthAccessToken();
    OAuthAccessToken(const QJsonObject& jsonObject);
    OAuthAccessToken(const OAuthAccessToken& otherToken);
    OAuthAccessToken& operator=(const OAuthAccessToken& otherToken);
    
    QByteArray authorizationHeaderValue() const { return QString("Bearer %1").arg(token).toUtf8(); }
     
    bool isExpired() const { return expiryTimestamp <= QDateTime::currentMSecsSinceEpoch(); }
    
    QString token;
    QString refreshToken;
    qlonglong expiryTimestamp;
    QString tokenType;
    
    friend QDataStream& operator<<(QDataStream &out, const OAuthAccessToken& token);
    friend QDataStream& operator>>(QDataStream &in, OAuthAccessToken& token);
private:
    void swap(OAuthAccessToken& otherToken);
};

#endif // hifi_OAuthAccessToken_h
