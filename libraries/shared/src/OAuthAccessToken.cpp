//
//  OAuthAccessToken.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/qdebug.h>

#include "OAuthAccessToken.h"

OAuthAccessToken::OAuthAccessToken() :
    token(),
    refreshToken(),
    expiryTimestamp(),
    tokenType()
{
    
}

OAuthAccessToken::OAuthAccessToken(const QJsonObject& jsonObject) :
    token(jsonObject["access_token"].toString()),
    refreshToken(jsonObject["refresh_token"].toString()),
    tokenType(jsonObject["token_type"].toString())
{
    qDebug() << "the refresh token is" << refreshToken;
}

OAuthAccessToken::OAuthAccessToken(const OAuthAccessToken& otherToken) {
    token = otherToken.token;
    refreshToken = otherToken.refreshToken;
    expiryTimestamp = otherToken.expiryTimestamp;
    tokenType = otherToken.tokenType;
}

OAuthAccessToken& OAuthAccessToken::operator=(const OAuthAccessToken& otherToken) {
    OAuthAccessToken temp(otherToken);
    swap(temp);
    return *this;
}

void OAuthAccessToken::swap(OAuthAccessToken& otherToken) {
    using std::swap;
    
    swap(token, otherToken.token);
    swap(refreshToken, otherToken.refreshToken);
    swap(expiryTimestamp, otherToken.expiryTimestamp);
    swap(tokenType, otherToken.tokenType);
}