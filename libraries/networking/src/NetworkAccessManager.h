//
//  NetworkAccessManager.h
//
//
//  Created by Clement on 7/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkAccessManager_h
#define hifi_NetworkAccessManager_h

#include <QNetworkAccessManager>

/// Wrapper around QNetworkAccessManager to restrict at one instance by thread
class NetworkAccessManager : public QObject {
    Q_OBJECT
public:
    static QNetworkAccessManager& getInstance();
};

#endif // hifi_NetworkAccessManager_h