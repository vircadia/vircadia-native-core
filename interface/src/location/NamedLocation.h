//
//  NamedLocation.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/1/14.
//
//

#ifndef __hifi__NamedLocation__
#define __hifi__NamedLocation__

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

    NamedLocation(const QJsonObject jsonData);

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

#endif /* defined(__hifi__NamedLocation__) */
