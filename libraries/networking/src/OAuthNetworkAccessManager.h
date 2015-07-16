//
//  OAuthNetworkAccessManager.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-09-18.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OAuthNetworkAccessManager_h
#define hifi_OAuthNetworkAccessManager_h

#include <QNetworkAccessManager>

class OAuthNetworkAccessManager : public QNetworkAccessManager {
public:
    static OAuthNetworkAccessManager* getInstance();
protected:
    virtual QNetworkReply* createRequest(Operation op, const QNetworkRequest& req, QIODevice* outgoingData = 0);
};

#endif // hifi_OAuthNetworkAccessManager_h