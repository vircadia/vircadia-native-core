//
//  waterHoseEntityScript.js
//  examples/homeContent/plant
//
//  Created by Eric Levin on 2/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for spraying water when a user holds down the trigger
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


(function() {

    Script.include("../../libraries/utils.js");

    var _this;
    var TWEEN = loadTween();
    WaterHose = function() {
        _this = this;
        this.potName = "plant pot";
        _this.potSearchRadius = 5;
 
    };

    WaterHose.prototype = {

        clickDownOnEntity: function() {
            // search for a pot with some seeds nearby
            var entities = Entities.findEntities(this.position, _this.potSearchRadius);
            entities.forEach(function(entity) {
                var name = Entities.getEntityProperties(entity, "name").name;
                if (name === _this.potName) {
                    // We've found out potted plant to grow!
                    _this.pottedPlant = entity;
                }
            });

        },
        holdingClickOnEntity: function() {
            if (!_this.pottedPlant) {
                // No plant nearby to grow, so return
                return;
            }
            Entities.callEntityMethod(_this.pottedPlant, "water");

        },

        preload: function(entityID) {
            print("EBL PRELOAD");
            this.entityID = entityID;
            this.props = Entities.getEntityProperties(this.entityID, ["position", "dimensions"]);
            this.position = this.props.position;
            this.dimensions = this.props.dimensions;
 
        },


        unload: function() {
            print("EBL UNLOAD DONE")
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new WaterHose();
});