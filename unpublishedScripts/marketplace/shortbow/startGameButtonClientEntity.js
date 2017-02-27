//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals utils */

(function() {
    Script.include('utils.js');

    function StartButton() {
    }
    StartButton.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            this.commChannel = "shortbow-" + Entities.getEntityProperties(entityID, 'parentID').parentID;
            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));
        },
        signalAC: function() {
            Messages.sendMessage(this.commChannel, JSON.stringify({
                type: 'start-game'
            }));
        },
        onCollide: function(entityA, entityB, collision) {
            var colliderName = Entities.getEntityProperties(entityB, 'name').name;

            if (colliderName.indexOf("projectile") > -1) {
                this.signalAC();
            }
        }
    };

    StartButton.prototype.startNearTrigger = StartButton.prototype.signalAC;
    StartButton.prototype.startFarTrigger = StartButton.prototype.signalAC;
    StartButton.prototype.clickDownOnEntity = StartButton.prototype.signalAC;

    return new StartButton();
});
