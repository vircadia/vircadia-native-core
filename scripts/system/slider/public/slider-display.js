//
//  slider-client.js
//
//  Created by kasenvr@gmail.com on 12 Jul 2020
//  Copyright 2020 Vircadia and contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    "use strict";
    this.entityID = null;
    var _this = this;

    // VARIABLES
    var presentationChannel = "default-presentation-channel";

    // APP EVENT AND MESSAGING ROUTING
    
    function onWebAppEventReceived(sendingEntityID, event) {
        if (sendingEntityID === _this.entityID) {
            var eventJSON = JSON.parse(event);
            if (eventJSON.app === "slider-display-web") { // This is our web app!
                // print("inventory.js received a web event: " + event);
        
                if (eventJSON.command === "ready") {
                    // console.info("Got init request message.");
                    initializeSliderDisplayApp();
                }
        
                if (eventJSON.command === "web-to-script-sync-state") {
                    // This data has to be stringified because userData only takes JSON strings and not actual objects.
                    console.log("web-to-script-sync-state" + JSON.stringify(eventJSON.data));
                    Entities.editEntity(_this.entityID, { "userData": JSON.stringify(eventJSON.data) });
                }
            }
        }
    }

    function sendToWeb(command, data) {
        var dataToSend = {
            "app": "slider-client-app",
            "command": command,
            "data": data
        };
        Entities.emitScriptEvent(_this.entityID, JSON.stringify(dataToSend));
    }
    
    function onMessageReceived(channel, message, sender, localOnly) {
        if (channel === presentationChannel) {
            var messageJSON = JSON.parse(message);
            if (messageJSON.command === "display-slide" ) { // We are receiving a slide.
                sendToWeb('script-to-web-display-slide', messageJSON.data);
            } 
        }
        print("Message received:");
        print("- channel: " + channel);
        print("- message: " + message);
        print("- sender: " + sender);
        print("- localOnly: " + localOnly);
    }
    
    // FUNCTIONS
    
    function initializeSliderDisplayApp () {
        var retrievedUserData = Entities.getEntityProperties(_this.entityID).userData;
        if (retrievedUserData != "") {
            retrievedUserData = JSON.parse(retrievedUserData);
        }        
        sendToWeb("script-to-web-initialize", { userData: retrievedUserData });
    }
    
    // Standard preload and unload, initialize the entity script here.
    
    this.preload = function (ourID) {
        this.entityID = ourID;
        
        Entities.webEventReceived.connect(onWebAppEventReceived);
        Messages.messageReceived.connect(onMessageReceived);
        Messages.subscribe(presentationChannel);
    };
    
    this.unload = function(entityID) {
        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(presentationChannel);
    };
    
});
