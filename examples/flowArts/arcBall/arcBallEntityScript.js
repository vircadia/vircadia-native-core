//  arcBallEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 12/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script handles the logic for the arcBall rave toy
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    var ArcBall = function() {
        _this = this;
    };

    ArcBall.prototype = {
        isGrabbed: false,
        startNearGrab: function() {
            //Search for nearby balls and create an arc to it if one is found
            var position = Entities.getEntityProperties(this.entityID, "position").position
            var entities = Entities.findEntities(position, 10);
            entities.forEach(function(entity) {
                var props = Entities.getEntityProperties(entity, ["position", "name"]);
                if (props.name === "Arc Ball" && JSON.stringify(_this.entityID) !== JSON.stringify(entity)) {
                    print ("WE FOUND ANOTHER ARC BALL");
                }
            })

        },

        continueNearGrab: function() {
        },

        releaseGrab: function() {
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new ArcBall();
});
