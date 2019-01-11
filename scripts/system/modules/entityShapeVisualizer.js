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
    "sphere": "Sphere",
    "box": "Cube",
    "ellipsoid": "Sphere",
    "cylinder-y": "Cylinder",
};

function getEntityShapePropertiesForType(properties) {
    switch (properties.type) {
        case "Zone":
            if (SHAPETYPE_TO_SHAPE[properties.shapeType]) {
                return {
                    type: "Shape",
                    shape: SHAPETYPE_TO_SHAPE[properties.shapeType]
                }
            } else if (properties.shapeType === "compound") {
                return {
                    type: "Model",
                    modelURL: properties.compoundShapeURL
                }
            }
            break;
    }

    // Default properties
    return {
        type: "Shape",
        shape: "Cube"
    }
}

function getStringifiedEntityShapePropertiesForType(properties) {
    return JSON.stringify(getEntityShapePropertiesForType(properties));
}

var REQUESTED_ENTITY_SHAPE_PROPERTIES = [
    'type', 'shapeType', 'compoundShapeURL', 'localDimensions'
];

function EntityShape(entityID) {
    this.entityID = entityID;
    var properties = Entities.getEntityProperties(entityID, REQUESTED_ENTITY_SHAPE_PROPERTIES);

    this.previousPropertiesForTypeStringified = getStringifiedEntityShapePropertiesForType(properties);

    this.initialize(properties, this.previousPropertiesForTypeStringified);
}

EntityShape.prototype = {
    initialize: function(properties, propertiesForTypeStringified) {
        // Create new instance of JS object:
        var overlayProperties = JSON.parse(propertiesForTypeStringified);

        overlayProperties.localPosition = Vec3.ZERO;
        overlayProperties.localRotation = Quat.IDENTITY;
        overlayProperties.localDimensions = properties.localDimensions;
        overlayProperties.canCastShadows = false;
        overlayProperties.parentID = this.entityID;
        overlayProperties.collisionless = true;
        this.entity = Entities.addEntity(overlayProperties, "local");

        console.warn("created " + this.entity);
        console.warn("SHAPETYPE = " + properties.shapeType);
        console.warn("SHAPE = " + Entities.getEntityProperties(this.entity, "shape").shape);


        this.materialEntity = Entities.addEntity({
            type: "Material",
            localPosition: Vec3.ZERO,
            localRotation: Quat.IDENTITY,
            localDimensions: properties.localDimensions,
            parentID: this.entity,
            priority: 1,
            materialURL: "materialData",
            materialData: JSON.stringify({
                materialVersion: 1,
                materials: {
                    albedo: [0.0, 0.0, 7.0],
                    unlit: true,
                    opacity: 0.4
                }
            }),
        }, "local");

    },

    update: function() {
        var properties = Entities.getEntityProperties(this.entityID, REQUESTED_ENTITY_SHAPE_PROPERTIES);
        var propertiesForTypeStringified = getStringifiedEntityShapePropertiesForType(properties);
        if (propertiesForTypeStringified !== this.previousPropertiesForTypeStringified) {
            this.previousPropertiesForTypeStringified = propertiesForTypeStringified;
            console.warn("Clearing old properties");
            this.clear();
            this.initialize(properties, propertiesForTypeStringified);
        } else {
            Entities.editEntity(this.entity, {
                localDimensions: properties.localDimensions,
            });
        }



        //this.previousProperties = Entities.getEntityProperties(this.entityID, REQUESTED_ENTITY_SHAPE_PROPERTIES);


        console.warn(JSON.stringify(this.previousProperties));
    },
    clear: function() {
        Entities.deleteEntity(this.materialEntity);
        Entities.deleteEntity(this.entity);
    }
};

function EntityShapeVisualizer(visualizedTypes) {
    this.acceptedEntities = [];
    this.ignoredEntities = [];
    this.entityShapes = {};

    this.visualizedTypes = visualizedTypes;
}

EntityShapeVisualizer.prototype = {
    addEntity: function(entityID, properties) {
        if (this.entityShapes[entityID]) {
            return;
        }
        this.entityShapes[entityID] = new EntityShape(entityID);

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
    updateSelection: function(selection) {
        var qualifiedSelection = selection.filter(function(entityID) {
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
        qualifiedSelection.forEach(function(entityID) {
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
            console.warn("removing " + entityID);
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
