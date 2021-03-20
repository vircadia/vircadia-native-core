"use strict";

//  entityShapeVisualizer.js
//
//  Created by Thijs Wenker on 1/11/19
//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var SHAPETYPE_TO_SHAPE = {
    "box": "Cube",
    "ellipsoid": "Sphere",
    "sphere": "Sphere",
    "cylinder-y": "Cylinder",
};

var REQUESTED_ENTITY_SHAPE_PROPERTIES = [
    'type', 'shapeType', 'compoundShapeURL', 'localDimensions'
];

function getEntityShapePropertiesForType(properties) {
    switch (properties.type) {
        case "Zone":
            if (SHAPETYPE_TO_SHAPE[properties.shapeType]) {
                return {
                    type: "Shape",
                    shape: SHAPETYPE_TO_SHAPE[properties.shapeType],
                    localDimensions: properties.localDimensions
                };
            } else if (properties.shapeType === "compound") {
                return {
                    type: "Model",
                    modelURL: properties.compoundShapeURL,
                    localDimensions: properties.localDimensions
                };
            }
            break;
    }

    // Default properties
    return {
        type: "Shape",
        shape: "Cube",
        localDimensions: properties.localDimensions
    };
}

function deepEqual(a, b) {
    if (a === b) {
        return true;
    }

    if (typeof(a) !== "object" || typeof(b) !== "object") {
        return false;
    }

    if (Object.keys(a).length !== Object.keys(b).length) {
        return false;
    }

    for (var property in a) {
        if (!a.hasOwnProperty(property)) {
            continue;
        }
        if (!b.hasOwnProperty(property)) {
            return false;
        }
        if (!deepEqual(a[property], b[property])) {
            return false;
        }
    }
    return true;
}

/**
 * Returns an array of property names which are different in comparison.
 * @param propertiesA
 * @param propertiesB
 * @returns {Array} - array of different property names
 */
function compareEntityProperties(propertiesA, propertiesB) {
    var differentProperties = [],
        property;

    for (property in propertiesA) {
        if (!propertiesA.hasOwnProperty(property)) {
            continue;
        }
        if (!propertiesB.hasOwnProperty(property) || !deepEqual(propertiesA[property], propertiesB[property])) {
            differentProperties.push(property);
        }
    }
    for (property in propertiesB) {
        if (!propertiesB.hasOwnProperty(property)) {
            continue;
        }
        if (!propertiesA.hasOwnProperty(property)) {
            differentProperties.push(property);
        }
    }

    return differentProperties;
}

function deepCopy(v) {
    return JSON.parse(JSON.stringify(v));
}

function EntityShape(entityID, entityShapeVisualizerSessionName) {
    this.entityID = entityID;
    this.entityShapeVisualizerSessionName = entityShapeVisualizerSessionName;

    var propertiesForType = getEntityShapePropertiesForType(Entities.getEntityProperties(entityID, REQUESTED_ENTITY_SHAPE_PROPERTIES));

    this.previousPropertiesForType = propertiesForType;
    
    this.initialize(propertiesForType);
}

EntityShape.prototype = {
    initialize: function(properties) {
        // Create new instance of JS object:
        var overlayProperties = deepCopy(properties);

        overlayProperties.name = this.entityShapeVisualizerSessionName;
        overlayProperties.localPosition = Vec3.ZERO;
        overlayProperties.localRotation = Quat.IDENTITY;
        overlayProperties.canCastShadows = false;
        overlayProperties.parentID = this.entityID;
        overlayProperties.collisionless = true;
        overlayProperties.ignorePickIntersection = true;
        this.entity = Entities.addEntity(overlayProperties, "local");
        var PROJECTED_MATERIALS = false;
        this.materialEntity = Entities.addEntity({
            type: "Material",
            name: "MATERIAL_" + this.entityShapeVisualizerSessionName,
            localPosition: Vec3.ZERO,
            localRotation: Quat.IDENTITY,
            localDimensions: properties.localDimensions,
            parentID: this.entity,
            priority: 1,
            materialMappingMode: PROJECTED_MATERIALS ? "projected" : "uv",
            materialURL: Script.resolvePath("../../assets/images/materials/GridPattern.json"),
            ignorePickIntersection: true
        }, "local");
    },
    update: function() {
        var propertiesForType = getEntityShapePropertiesForType(Entities.getEntityProperties(this.entityID, REQUESTED_ENTITY_SHAPE_PROPERTIES));

        var difference = compareEntityProperties(propertiesForType, this.previousPropertiesForType);

        if (deepEqual(difference, ['localDimensions'])) {
            this.previousPropertiesForType = propertiesForType;
            Entities.editEntity(this.entity, {
                localDimensions: propertiesForType.localDimensions,
            });
        } else if (difference.length > 0) {
            this.previousPropertiesForType = propertiesForType;
            this.clear();
            this.initialize(propertiesForType);
        }
    },
    clear: function() {
        Entities.deleteEntity(this.materialEntity);
        Entities.deleteEntity(this.entity);
    }
};

function EntityShapeVisualizer(visualizedTypes, entityShapeVisualizerSessionName) {
    this.acceptedEntities = [];
    this.ignoredEntities = [];
    this.entityShapes = {};
    this.entityShapeVisualizerSessionName = entityShapeVisualizerSessionName;
    this.visualizedTypes = visualizedTypes;
}

EntityShapeVisualizer.prototype = {
    addEntity: function(entityID) {
        if (this.entityShapes[entityID]) {
            return;
        }
        this.entityShapes[entityID] = new EntityShape(entityID, this.entityShapeVisualizerSessionName);

    },
    updateEntity: function(entityID) {
        if (!this.entityShapes[entityID]) {
            return;
        }
        this.entityShapes[entityID].update();
    },
    removeEntity: function(entityID) {
        if (!this.entityShapes[entityID]) {
            return;
        }
        this.entityShapes[entityID].clear();
        delete this.entityShapes[entityID];
    },
    cleanup: function() {
        Object.keys(this.entityShapes).forEach(function(entityID) {
            this.entityShapes[entityID].clear();
        }, this);
        this.entityShapes = {};
    },
    setEntities: function(entities) {
        var qualifiedEntities = entities.filter(function(entityID) {
            if (this.acceptedEntities.indexOf(entityID) !== -1) {
                return true;
            }
            if (this.ignoredEntities.indexOf(entityID) !== -1) {
                return false;
            }
            if (this.visualizedTypes.indexOf(Entities.getEntityProperties(entityID, "type").type) !== -1) {
                this.acceptedEntities.push(entityID);
                return true;
            }
            this.ignoredEntities.push(entityID);
            return false;
        }, this);


        var newEntries = [];
        var updateEntries = [];

        var currentEntries = Object.keys(this.entityShapes);
        qualifiedEntities.forEach(function(entityID) {
            if (currentEntries.indexOf(entityID) !== -1) {
                updateEntries.push(entityID);
            } else {
                newEntries.push(entityID);
            }
        });

        var deleteEntries = currentEntries.filter(function(entityID) {
            return updateEntries.indexOf(entityID) === -1;
        });

        deleteEntries.forEach(function(entityID) {
            this.removeEntity(entityID);
        }, this);

        updateEntries.forEach(function(entityID) {
            this.updateEntity(entityID);
        }, this);

        newEntries.forEach(function(entityID) {
            this.addEntity(entityID);
        }, this);
    }
};

module.exports = EntityShapeVisualizer;
