//
//  LocationManager.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/7/14.
//
//

#ifndef __hifi__LocationManager__
#define __hifi__LocationManager__

#include <QtCore>
#include "NamedLocation.h"

class LocationManager : public QObject {
    Q_OBJECT

public:
    enum LocationCreateResponse {
        Created,
        AlreadyExists
    };

    LocationManager() { };
    void createNamedLocation(QString locationName, QString creator, glm::vec3 location, glm::quat orientation);

signals:
    void creationCompleted(LocationManager::LocationCreateResponse response, NamedLocation* location);

private:
    NamedLocation* _namedLocation;
    
private slots:
    void locationDataReceived(bool locationExists);
};

#endif /* defined(__hifi__LocationManager__) */
