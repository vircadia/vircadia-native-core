//
//  growingPlantEntityScript.js
//  examples/homeContent/plant
//
//  Created by Eric Levin on 2/10/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for growing a plant when it has water poured on it
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



(function() {
    Script.include("../../libraries/utils.js");

    var _this;
    GrowingPlant = function() {
        _this = this;

    };

    GrowingPlant.prototype = {


        grow: function() {
            print(" I AM BEING GROWN RIGHT NOW")
        },

        preload: function(entityID) {
            _this.entityID = entityID;


        },

    

        unload: function() {

    
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});