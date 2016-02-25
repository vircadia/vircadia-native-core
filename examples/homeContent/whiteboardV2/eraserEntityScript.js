//
//  eraserEntityScript.js
//  examples/homeContent/eraserEntityScript
//
//  Created by Eric Levin on 2/17/15.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script provides logic for an object with attached script to erase nearby marker strokes
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



(function() {
    Script.include("../../libraries/utils.js");
    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];
    var _this;
    Eraser = function() {
        _this = this;

        _this.ERASER_TRIGGER_THRESHOLD = 0.2;
        _this.STROKE_NAME = "hifi-marker-stroke";
        _this.ERASER_TO_STROKE_SEARCH_RADIUS = 0.7;
    };

    Eraser.prototype = {

        startEquip: function(id, params) {
            _this.equipped = true;
            _this.hand = params[0] == "left" ? 0 : 1;
            // We really only need to grab position of marker strokes once, and then just check to see if eraser comes near enough to those strokes 

        },
        continueEquip: function() {
            this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[_this.hand]);
            if (_this.triggerValue > _this.ERASER_TRIGGER_THRESHOLD) {
                _this.continueHolding();
            }
        },


        continueHolding: function() {
            var eraserPosition = Entities.getEntityProperties(_this.entityID, "position").position;
            var strokeIDs = Entities.findEntities(eraserPosition, _this.ERASER_TO_STROKE_SEARCH_RADIUS);
            // Create a map of stroke entities and their positions

            strokeIDs.forEach(function(strokeID) {
                var strokeProps = Entities.getEntityProperties(strokeID, ["position", "name"]);
                if (strokeProps.name === _this.STROKE_NAME && Vec3.distance(eraserPosition, strokeProps.position) < _this.ERASER_TO_STROKE_SEARCH_RADIUS) {
                    Entities.deleteEntity(strokeID);
                }
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;

        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Eraser();
});