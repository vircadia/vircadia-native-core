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

// #include "DataServerClient.h"

class NamedLocation : public QObject { //, public DataServerCallbackObject, public DataServerCallerObject {
    Q_OBJECT
    
public:
    
    NamedLocation(QString locationName, QString createdBy, glm::vec3 location, glm::quat orientation) {
        _locationName = locationName;
        _createdBy = createdBy;
        _location = location;
        _orientation = orientation;
    }

    bool isEmpty() { return _locationName.isNull() || _locationName.isEmpty(); }

    // DataServerCallbackObject implementation
    void processDataServerResponse(const QString& userString, const QStringList& keyList, const QStringList& valueList);

    // DataServerCallerObject implementation
    QHash<QString, QString> getHashData();

    // properties >>
    void setLocationName(QString locationName) { _locationName = locationName; }
    QString locationName() { return _locationName; }
    
    void createdBy(QString createdBy) { _createdBy = createdBy; }
    QString getCreatedBy() { return _createdBy; }
    
    void setLocation(glm::vec3 location) { _location = location; }
    glm::vec3 location() { return _location; }
    
    void setOrientation(glm::quat orentation) { _orientation = orentation; }
    glm::quat orientation() { return _orientation; }
    // properties <<
    
signals:
    void dataReceived(bool locationExists);

private:
    QString _locationName;
    QString _createdBy;
    glm::vec3 _location;
    glm::quat _orientation;

};

#endif /* defined(__hifi__NamedLocation__) */
