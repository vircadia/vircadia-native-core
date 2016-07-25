//  Created by James B. Pollack @imgntn 6/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

(function() {

    var _this;

    Swiper = function() {
        _this = this;
    }

    Swiper.prototype = {
        busy: false,
        preload: function(entityID) {
            this.entityID = entityID;
            Script.update.connect(this.update);
        },
        clickReleaseOnEntity: function() {
            this.createSupplies();
        },
        update: function() {
            if (_this.busy === true) {
                return;
            }

            var position = Entities.getEntityProperties(_this.entityID).position;
            var TRIGGER_THRESHOLD = 0.11;

            var leftHandPosition = MyAvatar.getLeftPalmPosition();
            var rightHandPosition = MyAvatar.getRightPalmPosition();

            var leftDistance = Vec3.distance(leftHandPosition, position)
            var rightDistance = Vec3.distance(rightHandPosition, position)

            if (rightDistance < TRIGGER_THRESHOLD || leftDistance < TRIGGER_THRESHOLD) {
                _this.createSupplies();
                _this.busy = true;
                Script.setTimeout(function() {
                    _this.busy = false;
                }, 2000)
            }
        },

        createSupplies: function() {
            var myProperties = Entities.getEntityProperties(this.entityID);

            Entities.callEntityMethod(myProperties.parentID, "createMarkers");
            Entities.callEntityMethod(myProperties.parentID, "createEraser");

        },
        unload: function() {
            Script.update.disconnect(this.update);
        }

    }
    return new Swiper
})