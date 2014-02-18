//
//  OAuthAccessToken.h
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__OAuthAccessToken__
#define __hifi__OAuthAccessToken__

#include <QtCore/QObject>
#include <QtCore/QDateTime>

class OAuthAccessToken : public QObject {
    Q_OBJECT
public:
    OAuthAccessToken();
    OAuthAccessToken(const OAuthAccessToken& otherToken);
    OAuthAccessToken& operator=(const OAuthAccessToken& otherToken);
    
    bool isExpired() { return expiryTimestamp <= QDateTime::currentMSecsSinceEpoch(); }
    
    QString token;
    QString refreshToken;
    quint64 expiryTimestamp;
    QString tokenType;
private:
    void swap(OAuthAccessToken& otherToken);
};

#endif /* defined(__hifi__OAuthAccessToken__) */
