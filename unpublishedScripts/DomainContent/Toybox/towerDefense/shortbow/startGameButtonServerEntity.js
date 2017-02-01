(function() {
    Script.include('utils.js');

    Button = function() {
    };
    Button.prototype = {
        preload: function(entityID) {
            print("Loaded enemy entity");
            this.entityID = entityID;

            var props = Entities.getEntityProperties(this.entityID, 'parentID');
            this.gameChannel = 'button-' + props.parentID;

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
                    visible: message === 'show'
                });
            }
        },
    };

    return new Button();
});
