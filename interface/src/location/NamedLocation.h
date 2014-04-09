//
//  NamedLocation.h
//  interface/src/location
//
//  Created by Stojce Slavkovski on 2/1/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NamedLocation_h
#define hifi_NamedLocation_h

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore>

class NamedLocation : public QObject {
    Q_OBJECT
    
public:
    NamedLocation(QString locationName, glm::vec3 location, glm::quat orientation, QString domain) {
        _locationName = locationName;
        _location = location;
        _orientation = orientation;
        _domain = domain;
    }

    QString toJsonString();

    bool isEmpty() { return _locationName.isNull() || _locationName.isEmpty(); }

    void setLocationName(QString locationName) { _locationName = locationName; }
    QString locationName() { return _locationName; }

    void setLocation(glm::vec3 location) { _location = location; }
    glm::vec3 location() { return _location; }
    
    void setOrientation(glm::quat orentation) { _orientation = orentation; }
    glm::quat orientation() { return _orientation; }

    void setDomain(QString domain) { _domain = domain; }
    QString domain() { return _domain; }

signals:
    void dataReceived(bool locationExists);

private:
    
    QString _locationName;
    QString _createdBy;
    glm::vec3 _location;
    glm::quat _orientation;
    QString _domain;

};

#endif // hifi_NamedLocation_h
