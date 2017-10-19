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

if (typeof Vec3.abs !== "function") {
    Vec3.abs = function (a) {
        return { x: Math.abs(a.x), y: Math.abs(a.y), z: Math.abs(a.z) };
    };
}

if (typeof Quat.ZERO !== "object") {
    // TODO: Change to Quat.IDENTITY.
    Quat.ZERO = Quat.fromVec3Radians(Vec3.ZERO);
}

if (typeof Uuid.NULL !== "string") {
    Uuid.NULL = "{00000000-0000-0000-0000-000000000000}";
}

if (typeof Uuid.SELF !== "string") {
    Uuid.SELF = "{00000000-0000-0000-0000-000000000001}";
}

if (typeof Entities.rootOf !== "function") {
    Entities.rootOfCache = {
        CACHE_ENTRY_EXPIRY_TIME: 1000 // ms
    };

    Entities.rootOf = function (entityID) {
        if (Entities.rootOfCache[entityID]) {
            if (Date.now() - Entities.rootOfCache[entityID].timeStamp
                    < Entities.rootOfCache.CACHE_ENTRY_EXPIRY_TIME) {
                return Entities.rootOfCache[entityID].rootOf;
            }
            delete Entities.rootOfCache[entityID];
        }

        var rootEntityID,
            entityProperties,
            PARENT_PROPERTIES = ["parentID"];
        rootEntityID = entityID;
        entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
        while (entityProperties.parentID && entityProperties.parentID !== Uuid.NULL) {
            rootEntityID = entityProperties.parentID;
            entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
        }

        Entities.rootOfCache[entityID] = {
            rootOf: rootEntityID,
            timeStamp: Date.now()
        };
        return rootEntityID;
    };
}

if (typeof Entities.hasEditableRoot !== "function") {
    Entities.hasEditableRootCache = {
        CACHE_ENTRY_EXPIRY_TIME: 5000 // ms
    };

    Entities.hasEditableRoot = function (entityID) {
        if (Entities.hasEditableRootCache[entityID]) {
            if (Date.now() - Entities.hasEditableRootCache[entityID].timeStamp
                    < Entities.hasEditableRootCache.CACHE_ENTRY_EXPIRY_TIME) {
                return Entities.hasEditableRootCache[entityID].hasEditableRoot;
            }
            delete Entities.hasEditableRootCache[entityID];
        }

        var EDITIBLE_ENTITY_QUERY_PROPERTYES = ["parentID", "visible", "locked", "type"],
            NONEDITABLE_ENTITY_TYPES = ["Unknown", "Zone", "Light"],
            properties,
            isEditable;

        properties = Entities.getEntityProperties(entityID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
        while (properties.parentID && properties.parentID !== Uuid.NULL) {
            properties = Entities.getEntityProperties(properties.parentID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
        }
        isEditable = properties.visible && !properties.locked && NONEDITABLE_ENTITY_TYPES.indexOf(properties.type) === -1;

        Entities.hasEditableRootCache[entityID] = {
            hasEditableRoot: isEditable,
            timeStamp: Date.now()
        };
        return isEditable;
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
        if (a === "{}") {
            return JSON.parse(b); // Always return a new object.
        }
        if (b === "{}") {
            return JSON.parse(a); // ""
        }
        return JSON.parse(a.slice(0, -1) + "," + b.slice(1));
    };
}
