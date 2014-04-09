//
//  NamedLocation.cpp
//  interface/src/location
//
//  Created by Stojce Slavkovski on 2/1/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
