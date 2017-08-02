//
//  utilities.js
//
//  Created by David Rowe on 21 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

if (typeof Vec3.min !== "function") {
    Vec3.min = function (a, b) {
        return { x: Math.min(a.x, b.x), y: Math.min(a.y, b.y), z: Math.min(a.z, b.z) };
    };
}

if (typeof Vec3.max !== "function") {
    Vec3.max = function (a, b) {
        return { x: Math.max(a.x, b.x), y: Math.max(a.y, b.y), z: Math.max(a.z, b.z) };
    };
}

if (typeof Entities.rootOf !== "function") {
    Entities.rootOf = function (entityID) {
        var rootEntityID,
            entityProperties,
            PARENT_PROPERTIES = ["parentID"],
            NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
        rootEntityID = entityID;
        entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
        while (entityProperties.parentID && entityProperties.parentID !== NULL_UUID) {
            rootEntityID = entityProperties.parentID;
            entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
        }
        return rootEntityID;
    };
}

if (typeof Entities.hasEditableRoot !== "function") {
    Entities.hasEditableRoot = function (entityID) {
        var EDITIBLE_ENTITY_QUERY_PROPERTYES = ["parentID", "visible", "locked", "type"],
            NONEDITABLE_ENTITY_TYPES = ["Unknown", "Zone", "Light"],
            NULL_UUID = "{00000000-0000-0000-0000-000000000000}",
            properties;
        properties = Entities.getEntityProperties(entityID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
        while (properties.parentID && properties.parentID !== NULL_UUID) {
            properties = Entities.getEntityProperties(properties.parentID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
        }
        return properties.visible && !properties.locked && NONEDITABLE_ENTITY_TYPES.indexOf(properties.type) === -1;
    };
}

if (typeof Object.clone !== "function") {
    Object.clone = function (object) {
        return JSON.parse(JSON.stringify(object));
    };
}

if (typeof Object.merge !== "function") {
    Object.merge = function (objectA, objectB) {
        var a = JSON.stringify(objectA),
            b = JSON.stringify(objectB);
        return JSON.parse(a.slice(0, -1) + "," + b.slice(1));
    };
}

