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

#include <AddressManager.h>
#include <UUID.h>

#include "NamedLocation.h"

NamedLocation::NamedLocation(const QString& name,
                             const glm::vec3& position, const glm::quat& orientation,
                             const QUuid& domainID) :
    _name(name),
    _position(position),
    _orientation(orientation),
    _domainID(domainID)
{
    
}

const QString JSON_FORMAT = "{\"location\":{\"path\":\"%1\",\"domain_id\":\"%2\",\"name\":\"%3\"}}";

QString NamedLocation::toJsonString() {
    return JSON_FORMAT.arg(AddressManager::pathForPositionAndOrientation(_position, true, _orientation),
                           uuidStringWithoutCurlyBraces(_domainID), _name);
}
