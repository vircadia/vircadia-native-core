//
// prevent-erase-in-zone-example.js
// 
//
// Created by Brad Hefta-Gaub to use Entities on Jan. 25, 2018
// Copyright 2018 High Fidelity, Inc.
//
// This sample entity edit filter script will keep prevent any entity inside the zone from being erased.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function filter(properties, type, originalProperties, zoneProperties) {    

    if (type == Entities.ERASE_FILTER_TYPE) {
        return false;
    }
    return properties;
}

filter.wantsOriginalProperties = true;
filter.wantsZoneProperties = true;
filter;