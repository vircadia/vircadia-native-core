//
//  OAuthAccessToken.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>

#include "OAuthAccessToken.h"

OAuthAccessToken::OAuthAccessToken() :
    token(),
    refreshToken(),
    expiryTimestamp(-1),
    tokenType()
{
    
}

OAuthAccessToken::OAuthAccessToken(const QJsonObject& jsonObject) :
    token(jsonObject["access_token"].toString()),
    refreshToken(jsonObject["refresh_token"].toString()),
    expiryTimestamp(QDateTime::currentMSecsSinceEpoch() + (jsonObject["expires_in"].toDouble() * 1000)),
    tokenType(jsonObject["token_type"].toString())
{
    
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

QDataStream& operator<<(QDataStream &out, const OAuthAccessToken& token) {
    out << token.token << token.expiryTimestamp << token.tokenType << token.refreshToken;
    return out;
}

QDataStream& operator>>(QDataStream &in, OAuthAccessToken& token) {
    in >> token.token >> token.expiryTimestamp >> token.tokenType >> token.refreshToken;
    return in;
}
