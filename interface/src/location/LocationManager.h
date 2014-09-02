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

#include "AccountManager.h"
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

    LocationManager();
    void createNamedLocation(NamedLocation* namedLocation);

    void goTo(QString destination);
    void goToUser(QString userName);
    void goToPlace(QString placeName);
    void goToOrientation(QString orientation);
    bool goToDestination(QString destination);
    
public slots:
    void handleAddressLookupError(QNetworkReply::NetworkError networkError, const QString& errorString);

private:
    void replaceLastOccurrence(const QChar search, const QChar replace, QString& string);

signals:
    void creationCompleted(LocationManager::NamedLocationCreateResponse response);
    void multipleDestinationsFound(const QJsonObject& userData, const QJsonObject& placeData);
    
private slots:
    void namedLocationDataReceived(const QJsonObject& data);
    void errorDataReceived(QNetworkReply::NetworkError error, const QString& message);
    void goToAddressFromResponse(const QJsonObject& jsonObject);

};

#endif // hifi_LocationManager_h
