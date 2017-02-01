(function() {
    Script.include('utils.js');

    Display = function() {
    };
    Display.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;

            this.entityIDsThatHaveCollidedWithMe = [];

            var props = Entities.getEntityProperties(this.entityID, ['userData', 'parentID']);
            var data = utils.parseJSON(props.userData);
            if (data !== undefined && data.displayType !== undefined) {
                this.gameChannel = data.displayType + "-" + props.parentID;
            } else {
                print("scoreboardEntity.js | ERROR: userData does not contain a game channel and/or team number");
            }

            Messages.subscribe(this.gameChannel);
            Messages.messageReceived.connect(this, this.onReceivedMessage);
        },
        unload: function() {
            Messages.unsubscribe(this.gameChannel);
            Messages.messageReceived.disconnect(this, this.onReceivedMessage);
        },
        onReceivedMessage: function(channel, message, senderID) {
            if (channel === this.gameChannel) {
                Entities.editEntity(this.entityID, {
                    text: message
                });
            }
        },
    };

    return new Display();
});
