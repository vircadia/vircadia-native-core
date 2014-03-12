//
//  LocationManager.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/1/14.
//
//

#include "NamedLocation.h"

const QString JSON_FORMAT = "{\"address\":{\"position\":\"%1,%2,%3\","
                            "\"orientation\":\"%4,%5,%6,%7\",\"domain\":\"%8\"},\"name\":\"%9\"}";

QString NamedLocation::toJsonString() {
    return JSON_FORMAT.arg(QString::number(_location.x),
                                        QString::number(_location.y),
                                        QString::number(_location.z),
                                        QString::number(_orientation.w),
                                        QString::number(_orientation.x),
                                        QString::number(_orientation.y),
                                        QString::number(_orientation.z),
                                        _domain,
                                        _locationName);
}

NamedLocation::NamedLocation(const QJsonObject jsonData) {

    bool hasProperties;

    if (jsonData.contains("name")) {
        hasProperties = true;
        _locationName = jsonData["name"].toString();
    }

    if (jsonData.contains("username")) {
        hasProperties = true;
        _createdBy = jsonData["username"].toString();
    }

    if (jsonData.contains("domain")) {
        hasProperties = true;
        _domain = jsonData["domain"].toString();
    }

    // parse position
    if (jsonData.contains("position")) {
        hasProperties = true;
        const int NUMBER_OF_POSITION_ITEMS = 3;
        const int X_ITEM = 0;
        const int Y_ITEM = 1;
        const int Z_ITEM = 2;

        QStringList coordinateItems = jsonData["position"].toString().split(",", QString::SkipEmptyParts);
        if (coordinateItems.size() == NUMBER_OF_POSITION_ITEMS) {
            double x = coordinateItems[X_ITEM].trimmed().toDouble();
            double y = coordinateItems[Y_ITEM].trimmed().toDouble();
            double z = coordinateItems[Z_ITEM].trimmed().toDouble();
            _location = glm::vec3(x, y, z);
        }
    }

    // parse orientation
    if (jsonData.contains("orientation")) {
        hasProperties = true;
        QStringList orientationItems = jsonData["orientation"] .toString().split(",", QString::SkipEmptyParts);
        const int NUMBER_OF_ORIENTATION_ITEMS = 4;
        const int W_ITEM = 0;
        const int X_ITEM = 1;
        const int Y_ITEM = 2;
        const int Z_ITEM = 3;

        if (orientationItems.size() == NUMBER_OF_ORIENTATION_ITEMS) {
            double w = orientationItems[W_ITEM].trimmed().toDouble();
            double x = orientationItems[X_ITEM].trimmed().toDouble();
            double y = orientationItems[Y_ITEM].trimmed().toDouble();
            double z = orientationItems[Z_ITEM].trimmed().toDouble();
            _orientation = glm::quat(w, x, y, z);
        }
    }
}