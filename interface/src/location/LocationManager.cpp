//
//  LocationManager.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/7/14.
//
//

#include "LocationManager.h"

void LocationManager::createNamedLocation(QString locationName, QString creator, glm::vec3 location, glm::quat orientation) {
    _namedLocation = new NamedLocation(locationName, creator, location, orientation);
    connect(_namedLocation, SIGNAL(dataReceived(bool)), SLOT(locationDataReceived(bool)));
    // DataServerClient::getHashFieldsForKey(DataServerKey::NamedLocation, _namedLocation->locationName(), _namedLocation);
}

void LocationManager::locationDataReceived(bool locationExists) {
    disconnect(_namedLocation, SIGNAL(dataReceived(bool)), this, SLOT(locationDataReceived(bool)));
    if (locationExists) {
        emit creationCompleted(AlreadyExists, _namedLocation);
    } else {
        // DataServerClient::putHashFieldsForKey(DataServerKey::NamedLocation, _namedLocation->locationName(), _namedLocation);
        emit creationCompleted(Created, _namedLocation);
    }
}

