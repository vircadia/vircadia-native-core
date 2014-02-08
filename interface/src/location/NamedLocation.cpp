//
//  LocationManager.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/1/14.
//
//

#include "NamedLocation.h"

// deserialize data
void NamedLocation::processDataServerResponse(const QString& userString,
                                              const QStringList& keyList,
                                              const QStringList& valueList) {
    for (int i = 0; i < keyList.count(); i++) {
        if (keyList[i] == "creator") {
            _createdBy = valueList[i];
        } else if (keyList[i] == "location") {
            QStringList locationCoords = valueList[i].split(",");
            if (locationCoords.length() == 3) {
                _location = glm::vec3(locationCoords[0].toLong(), locationCoords[1].toLong(), locationCoords[2].toLong());
            }
        } else if (keyList[i] == "orientation") {
            
            QStringList orientationCoords = valueList[i].split(",");
            if (orientationCoords.length() == 4) {
                _orientation = glm::quat(orientationCoords[0].toLong(),
                                         orientationCoords[1].toLong(),
                                         orientationCoords[2].toLong(),
                                         orientationCoords[3].toLong());
            }
        }
    }
    
    emit dataReceived(keyList.count() > 0);
}

// serialize data
QHash<QString, QString> NamedLocation::getHashData() {
    QHash<QString, QString> response;
    qDebug() << QString::fromStdString(glm::to_string(_location));
    response["location"] = QString::number(_location.x) + "," + QString::number(_location.y) + "," + QString::number(_location.z);
    response["creator"] = _createdBy;
    response["orientation"] = QString::number(_orientation.x) + "," + QString::number(_orientation.y) + "," + QString::number(_orientation.z) + "," + QString::number(_orientation.w);
    return response;
}