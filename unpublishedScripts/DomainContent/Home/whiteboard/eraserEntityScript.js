//
//  eraserEntityScript.js
//
//  Created by Eric Levin on 2/17/15.
//  Additions by James B. Pollack @imgntn 6/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script provides logic for an object with attached script to erase nearby marker strokes
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    Script.include('atp:/whiteboard/utils.js');

    var _this;

    Eraser = function() {
        _this = this;
        _this.STROKE_NAME = "hifi_polyline_markerStroke";
        _this.ERASER_TO_STROKE_SEARCH_RADIUS = 0.1;
    };

    Eraser.prototype = {

        continueNearGrab: function() {
            _this.eraserPosition = Entities.getEntityProperties(_this.entityID, "position").position;
            _this.continueHolding();
        },

        continueHolding: function() {
            var results = Entities.findEntities(_this.eraserPosition, _this.ERASER_TO_STROKE_SEARCH_RADIUS);
            // Create a map of stroke entities and their positions

            results.forEach(function(stroke) {
                var props = Entities.getEntityProperties(stroke, ["position", "name"]);
                if (props.name === _this.STROKE_NAME && Vec3.distance(_this.eraserPosition, props.position) < _this.ERASER_TO_STROKE_SEARCH_RADIUS) {
                    Entities.deleteEntity(stroke);
                }
            });
        },

        preload: function(entityID) {
            _this.entityID = entityID;

        },

        startEquip: function() {
            _this.startNearGrab();
        },

        continueEquip: function() {
            _this.continueNearGrab();
        },


    };

    return new Eraser();
});