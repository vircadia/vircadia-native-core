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

    function Enemy() {
    }
    Enemy.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
            var data = utils.parseJSON(userData);
            if (data !== undefined && data.gameChannel !== undefined) {
                this.gameChannel = data.gameChannel;
            } else {
                print("enemyServerEntity.js | ERROR: userData does not contain a game channel and/or team number");
            }
            var self = this;
            this.heartbeatTimerID = Script.setInterval(function() {
                Messages.sendMessage(self.gameChannel, JSON.stringify({
                    type: "enemy-heartbeat",
                    entityID: self.entityID,
                    position: Entities.getEntityProperties(self.entityID, 'position').position
                }));
            }, 1000);
        },
        unload: function() {
            Script.clearInterval(this.heartbeatTimerID);
        }
    };

    return new Enemy();
});
