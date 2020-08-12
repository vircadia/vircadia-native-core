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
    var lastMessageReceivedData = null;
    var readyToSendAgain = true;
    
    // We use a timeout because it's possible the server hasn't fully saved our data to userData or ATP.
    var PROCESSING_MSG_DEBOUNCE_TIME = 200; // 200ms
    // Upon receiving a message, we will wait before allowing any updates 
    // to be sent as it's possible our app will try to send a repeat of the 
    // same message that was sent to all clients prior. 
    // This will result in a loop if we do not prevent it.
    var SENDING_MESSAGE_DEBOUNCE_TIME = 800; // 800ms

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
                    // console.log("web-to-script-sync-state" + JSON.stringify(eventJSON.data));
                    presentationChannel = eventJSON.data.presentationChannel;
                    saveState(eventJSON.data);
                }
                    
                if (eventJSON.command === "web-to-script-slide-changed") {
                    // console.log("web-to-script-slide-changed:" + eventJSON.data);
                    var dataPacket = {
                        command: "display-slide",
                        data: eventJSON.data
                    }
                    sendMessage(dataPacket);
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

    function sendMessage(dataToSend) {
        dataToSend.data.senderEntityID = _this.entityID;
        dataToSend.data.senderUUID = MyAvatar.sessionUUID;
        // console.log("Sending message from client:" + JSON.stringify(dataToSend));
        // console.log("On channel:" + presentationChannel);
        // console.log("lastMessageReceivedData: "+ JSON.stringify(lastMessageReceivedData));
        // if (dataToSend.command === "display-slide" && lastMessageReceivedData) {
        //     if (dataToSend.data.slide !== lastMessageReceivedData.slide 
        //         || dataToSend.data.currentSlide !== lastMessageReceivedData.currentSlide
        //         || dataToSend.data.slideChannel !== lastMessageReceivedData.slideChannel) {
        
        if (debounceSend()) {
            // We're trying to prevent any possible crazy loops.
            Messages.sendMessage(presentationChannel, JSON.stringify(dataToSend));
        }
    }
    
    // FUNCTIONS
    
    function updateFromStorage () {
        var retrievedUserData = Entities.getEntityProperties(_this.entityID).userData;
        if (retrievedUserData != "") {
            retrievedUserData = JSON.parse(retrievedUserData);
        }
        
        // console.log("THIS IS OUR RETRIEVAL PATH: " + retrievedUserData.atp.path);
        // console.log("IS VALID PATH: " + Assets.isValidFilePath(retrievedUserData.atp.path));
        
        if (retrievedUserData.atp && retrievedUserData.atp.use === true) {
            Assets.getAsset(
                {
                    url: retrievedUserData.atp.path,
                    responseType: "json"
                },
                function (error, result) {
                    if (error) {
                        // print("ERROR: Slide data not downloaded, bootstrapping for ATP use: "+ error);

                        sendToWeb("script-to-web-initialize", { userData: retrievedUserData });
                    } else {
                        if (result != "") {
                            // console.log("STRINGIFIED: " + JSON.stringify(result));
                            // result = JSON.parse(result);
                        }
                        
                        print("Retrieved DATA from from ATP.");
                        // print("Retrieved DATA FROM ATP: " + JSON.stringify(result.response));
                        
                        if (result.presentationChannel) {
                            // console.log("Triggering an update for presentation channel to:" + retrievedUserData.presentationChannel);
                            updatePresentationChannel(result.response.presentationChannel)
                        }
                        
                        result.response.atp = retrievedUserData.atp;

                        sendToWeb("script-to-web-initialize", { userData: result.response });
                    }
                }
            );
        } else {
            if (retrievedUserData.presentationChannel) {
                // console.log("Triggering an update for presentation channel to:" + retrievedUserData.presentationChannel);
                updatePresentationChannel(retrievedUserData.presentationChannel)
            }
            print("Retrieved DATA from userData.");
            // print("Retrieved DATA FROM USERDATA: " + retrievedUserData);
            sendToWeb("script-to-web-initialize", { userData: retrievedUserData });
        }
    }
    
    function initializeSliderClientApp () {
        updateFromStorage();
    }
    
    function debounceSend() {
        if(readyToSendAgain) {
            // console.log("Ready.");
            readyToSendAgain = false;
            Script.setTimeout(function() {
                readyToSendAgain = true;
            }, SENDING_MESSAGE_DEBOUNCE_TIME);
            return true;
        } else {
            // console.log("Not ready.");
            return false;
        }
    };
    
    function onMessageReceived(channel, message, sender, localOnly) {
        if (channel === presentationChannel) {
            var messageJSON = JSON.parse(message);
            if (messageJSON.command === "display-slide" ) { // We are receiving a slide.
                if (messageJSON.data.senderEntityID === _this.entityID && MyAvatar.sessionUUID != sender) {
                    // We got a message that this entity changed a slide, so let's update all instances of this entity for everyone.
                    lastMessageReceivedData = messageJSON.data;
                    updateFromStorage();
                    Script.setTimeout(function () {
                        debounceSend();
                        sendToWeb('script-to-web-update-slide-state', messageJSON.data);
                    }, PROCESSING_MSG_DEBOUNCE_TIME);
                }
                // console.log("FULL MESSAGE RECEIVED: " + JSON.stringify(messageJSON.data));
                // console.log("Who are they?" + sender);
                // console.log("Who are we? " + MyAvatar.sessionUUID);
            } 
        }
        // print("Message received on Slider Presenter App:");
        // print("- channel: " + channel);
        // print("- message: " + message);
        // print("- sender: " + sender);
        // print("- localOnly: " + localOnly);
    }
    
    function updatePresentationChannel (newChannel) {
        Messages.unsubscribe(presentationChannel);
        presentationChannel = newChannel;
        Messages.subscribe(presentationChannel);
    }
    
    function saveState (data) {
        // console.log("SAVING STATE: " + JSON.stringify(data));
        if (data.atp && data.atp.use === true) {
            // If ATP is activated, save there...
            // console.log("SAVING TO ATP!");
            
            Assets.putAsset(
                {
                    data: JSON.stringify(data),
                    path: data.atp.path
                },
                function (error, result) {
                    if (error) {
                        print("ERROR: Slider data not uploaded or mapping not set: " + error);
                    } else {
                        // print("Successfully saved: " + result.url); 
                    }
                }
            );
            
            // We want to add the latest ATP state back in whenever syncing.
            var retrievedUserData = Entities.getEntityProperties(_this.entityID).userData;
            if (retrievedUserData != "") {
                retrievedUserData = JSON.parse(retrievedUserData);
            }
            
            retrievedUserData.atp = data.atp;
            
            Entities.editEntity(_this.entityID, { "userData": JSON.stringify(retrievedUserData) });
        } else {
            // If ATP is not active, save all to userData...
            // console.log("NOT SAVING TO ATP!");
            // console.log("data.atp - " + JSON.stringify(data.atp));
            Entities.editEntity(_this.entityID, { "userData": JSON.stringify(data) });
        }
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
