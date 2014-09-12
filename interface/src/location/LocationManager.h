//
//  LocationManager.h
//  interface/src/location
//
//  Created by Stojce Slavkovski on 2/7/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocationManager_h
#define hifi_LocationManager_h

#include <QtCore>
#include <QtNetwork/QNetworkReply>

#include "NamedLocation.h"

class LocationManager : public QObject {
    Q_OBJECT

public:
    static LocationManager& getInstance();

    enum NamedLocationCreateResponse {
        Created,
        AlreadyExists,
        SystemError
    };

    void createNamedLocation(NamedLocation* namedLocation);

signals:
    void creationCompleted(const QString& errorMessage);
    
private slots:
    void namedLocationDataReceived(const QJsonObject& data);
    void errorDataReceived(QNetworkReply& errorReply);

};

#endif // hifi_LocationManager_h
