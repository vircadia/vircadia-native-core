//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include('utils.js');

    Display = function() {
    };
    Display.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;

            var props = Entities.getEntityProperties(this.entityID, ['userData', 'parentID']);
            var data = utils.parseJSON(props.userData);
            if (data !== undefined && data.displayType !== undefined) {
                this.commChannel = data.displayType + "-" + props.parentID;
            } else {
                print("scoreboardEntity.js | ERROR: userData does not contain a game channel and/or team number");
            }

            Messages.subscribe(this.commChannel);
            Messages.messageReceived.connect(this, this.onReceivedMessage);
        },
        unload: function() {
            Messages.unsubscribe(this.commChannel);
            Messages.messageReceived.disconnect(this, this.onReceivedMessage);
        },
        onReceivedMessage: function(channel, message, senderID) {
            if (channel === this.commChannel) {
                Entities.editEntity(this.entityID, {
                    text: message
                });
            }
        },
    };

    return new Display();
});
