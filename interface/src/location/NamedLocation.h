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
    NamedLocation(const QString& name, const glm::vec3& position, const glm::quat& orientation, const QUuid& domainID);

    QString toJsonString();

    bool isEmpty() { return _name.isNull() || _name.isEmpty(); }

    void setName(QString name) { _name = name; }
    const QString& getName() const { return _name; }

    void setLocation(glm::vec3 position) { _position = position; }
    const glm::vec3& getPosition() const { return _position; }
    
    void setOrientation(const glm::quat& orentation) { _orientation = orentation; }
    const glm::quat& getOrientation() const { return _orientation; }

    void setDomainID(const QUuid& domainID) { _domainID = domainID; }
    const QUuid& getDomainID() const { return _domainID; }

signals:
    void dataReceived(bool locationExists);

private:
    QString _name;
    QString _createdBy;
    glm::vec3 _position;
    glm::quat _orientation;
    QUuid _domainID;

};

#endif // hifi_NamedLocation_h
