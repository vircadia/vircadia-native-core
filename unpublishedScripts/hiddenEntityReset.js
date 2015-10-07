//
//  ResetSwitch.js
//
//  Created by Eric Levin on 10/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function () {

    var _this;

    var masterResetScript = Script.resolvePath("masterResetFromEntity.js");
    Script.include(masterResetScript);

    ResetSwitch = function () {
        _this = this;
    };

    ResetSwitch.prototype = {

        clickReleaseOnEntity: function (entityId, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.triggerReset();

        },

        startNearGrabNonColliding: function () {
            this.triggerReset();
        },

        triggerReset: function () {
            MasterReset();
        },

        preload: function (entityID) {
            this.entityID = entityID;
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new ResetSwitch();
})