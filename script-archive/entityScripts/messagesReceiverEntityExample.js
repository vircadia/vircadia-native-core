//
//  messagesReceiverEntityExample.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, will detect when the entity is being grabbed by the hydraGrab script
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var _this;

    var messageReceived = function (channel, message, senderID) {
        print("message received on channel:" + channel + ", message:" + message + ", senderID:" + senderID);
    };

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    MessagesReceiver = function () {
        _this = this;
    };

    MessagesReceiver.prototype = {

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * unsubscribing from messages
        //   * connectingf to the messageReceived signal
        preload: function (entityID) {
            this.entityID = entityID;

            print("---- subscribing ----");
            Messages.subscribe("example");
            Messages.messageReceived.connect(messageReceived);
        },

        // unload() will be called when the entity has become no longer known to the interface
        // it gives us a chance to clean up our local JavaScript object. In this case it means:
        //   * unsubscribing from messages
        //   * disconnecting from the messageReceived signal
        unload: function (entityID) {
            print("---- unsubscribing ----");
            Messages.unsubscribe("example");
            Messages.messageReceived.disconnect(messageReceived);
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new MessagesReceiver();
})
