(function() {
    Script.include('utils.js');

    Enemy = function() {
    };
    Enemy.prototype = {
        preload: function(entityID) {
            print("Loaded enemy entity");
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
                print("Sending heartbeat", self.gameChannel);
                Messages.sendMessage(self.gameChannel, JSON.stringify({
                    type: "enemy-heartbeat",
                    entityID: self.entityID,
                    position: Entities.getEntityProperties(self.entityID, 'position').position
                }));
            }, 1000);
        },
        unload: function() {
            Script.clearInterval(this.heartbeatTimerID);
        },
    };

    return new Enemy();
});
