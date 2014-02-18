//
//  AccountManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDataStream>
#include <QtCore/QMap>

#include "PacketHeaders.h"

#include "AccountManager.h"

AccountManager& AccountManager::getInstance() {
    static AccountManager sharedInstance;
    return sharedInstance;
}

AccountManager::AccountManager() :
    _username(),
    _accessTokens(),
    _networkAccessManager(NULL)
{
    
}

bool AccountManager::hasValidAccessTokenForRootURL(const QUrl &rootURL) {
    OAuthAccessToken accessToken = _accessTokens.value(rootURL);
    
    if (accessToken.token.isEmpty() || accessToken.isExpired()) {
        // emit a signal so somebody can call back to us and request an access token given a username and password
        qDebug() << "An access token is required for requests to" << qPrintable(rootURL.toString());
        emit authenticationRequiredForRootURL(rootURL);
        return false;
    } else {
        return true;
    }
}