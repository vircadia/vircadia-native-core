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
            if (eventJSON.app === "slider-client-web") { // This is our web app!
                // print("inventory.js received a web event: " + event);
        
                if (eventJSON.command === "ready") {
                    // console.info("Got init request message.");
                    initializeSliderClientApp();
                }
        
                if (eventJSON.command === "web-to-script-sync-state") {
                    // This data has to be stringified because userData only takes JSON strings and not actual objects.
                    console.log("web-to-script-sync-state" + JSON.stringify(eventJSON.data));
                    Entities.editEntity(_this.entityID, { "userData": JSON.stringify(eventJSON.data) });
                }
                    
                if (eventJSON.command === "web-to-script-slide-changed") {
                    sendMessage(eventJSON.data);
                    console.log("web-to-script-slide-changed");
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
        console.log("SENDING TO WEB ID:" + _this.entityID);
        Entities.emitScriptEvent(_this.entityID, JSON.stringify(dataToSend));
    }

    function sendMessage(dataToSend) {
        Messages.sendMessage(presentationChannel, JSON.stringify(dataToSend));
    }
    
    // FUNCTIONS
    
    function initializeSliderClientApp () {
        var retrievedUserData = JSON.parse(Entities.getEntityProperties(_this.entityID).userData);
        sendToWeb("script-to-web-initialize", { userData: retrievedUserData });
    }
    
    // Standard preload and unload, initialize the entity script here.
    
    this.preload = function (ourID) {
        this.entityID = ourID;
        print("Entity ID: " + this.entityID);
        
        Entities.webEventReceived.connect(onWebAppEventReceived);
    };
    
    this.unload = function(entityID) {
    };
    
});
