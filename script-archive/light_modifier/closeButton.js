//
//  closeButton.js
//
//  Created by James Pollack @imgntn on 12/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Entity script that closes sliders when interacted with.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    function CloseButton() {
        return this;
    }

    CloseButton.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            var entityProperties = Entities.getEntityProperties(this.entityID, "userData");
            this.initialProperties = entityProperties
            this.userData = JSON.parse(entityProperties.userData);
        },
        startNearGrab: function() {

        },
        startFarTrigger: function() {
            Messages.sendMessage('Hifi-Light-Modifier-Cleanup', 'callCleanup')
        }

    };

    return new CloseButton();
});