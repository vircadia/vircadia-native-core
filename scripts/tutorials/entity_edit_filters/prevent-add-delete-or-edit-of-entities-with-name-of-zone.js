//
// prevent-add-delete-or-edit-of-entities-with-name-of-zone.js
// 
//
// Created by Brad Hefta-Gaub to use Entities on Feb. 14, 2018
// Copyright 2018 High Fidelity, Inc.
//
// This sample entity edit filter script will get all edits, adds, physcis, and deletes, but will only block
// deletes, and will pass through all others.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function filter(properties, type, originalProperties, zoneProperties) {

    // for adds, check the new properties, if the name is the same as the zone, prevent the add
    // note: this case tests the example where originalProperties will be unknown, since as an
    // add message, there is no existing entity to get proprties from. But we need to make sure
    // zoneProperties is still set in the correct 4th parameter.
    if (type == Entities.ADD_FILTER_TYPE) {
        if (properties.name == zoneProperties.name) {
            return false;
        }
    } else {
        // for edits or deletes, check the "original" property for the entity against the zone's name
        if (originalProperties.name == zoneProperties.name) {
            return false;
        }
    }
    return properties;
}

filter.wantsToFilterAdd = true;  // do run on add
filter.wantsToFilterEdit = true;  // do not run on edit
filter.wantsToFilterPhysics = false;  // do not run on physics
filter.wantsToFilterDelete = true;  // do not run on delete
filter.wantsOriginalProperties = "name";
filter.wantsZoneProperties = "name";
filter;