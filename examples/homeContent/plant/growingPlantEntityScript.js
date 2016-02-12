//
//  growingPlantEntityScript.js
//  examples/homeContent/
//
//  Created by Eric Levin on 2/10/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for growing a plant when it has water poured on it
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


(function() {

    var _this;
    GrowingPlant = function() {
        _this = this;
    };

    GrowingPlant.prototype = {
      
        createLeaf: function() {
            var userData = JSON.stringify({
                ProceduralEntity: {
                    shaderUrl: "file:///C:/Users/Eric/hifi/examples/homeContent/plant/flower.fs",
                    // shaderUrl: "file:///C:/Users/Eric/hifi/examples/shaders/shaderToyWrapper.fs",
                }
            });
            this.leaf = Entities.addEntity({
                type: "Box",
                position: Vec3.sum(this.position, {x: 0, y: this.dimensions.y/2, z: 0 }),
                color: {red: 100, green: 10, blue: 100},
                dimensions: {x: 0.3, y: 0.001, z: 0.3},
                userData: userData
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.props = Entities.getEntityProperties(this.entityID, ["position", "dimensions"]);
            this.position = this.props.position;
            this.dimensions = this.props.dimensions;
            this.createLeaf();
        },

        unload: function() {
            Entities.deleteEntity(this.leaf);
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});