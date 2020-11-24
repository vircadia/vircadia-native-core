//
//  inventory.js
//
//  Created by Kalila L. - kasenvr@gmail.com on 2 Apr 2020
//  Copyright 2020 Vircadia and contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global AvatarList Clipboard console Controller Entities location Messages MyAvatar Script ScriptDiscoveryService Settings
Tablet Vec3 Window */

(function () { // BEGIN LOCAL_SCOPE
    "use strict";
    var AppUi = Script.require('appUi');
    var ui;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    // VARIABLES
    var APP_NAME = "INVENTORY";

    var inventoryDataSettingString = "inventoryApp.data";
    var inventoryData;

    var inventorySettingsString = "inventoryApp.settings";
    var inventorySettings;

    var INVENTORY_MESSAGES_CHANNEL = "com.vircadia.inventory";

    var RECEIVING_ITEM_QUEUE_LIMIT = 5;
    var receivingItemQueue = [];

    var NEARBY_USERS_SEARCH_RADIUS = 25;
    
    var isInventoryOpen = false;
    var appendItemWaiting = null;

    // APP EVENT AND MESSAGING ROUTING

    function onWebAppEventReceived(event) {
        var eventJSON = JSON.parse(event);
        if (eventJSON.app === "inventory") { // This is our web app!
            // print("inventory.js received a web event: " + event);
            
            if (eventJSON.command === "ready") {
                initializeInventoryApp();
            }
            
            if (eventJSON.command === "web-to-script-inventory") {
                receiveInventory(eventJSON.data);
            }
            
            if (eventJSON.command === "web-to-script-settings") {
                receiveSettings(eventJSON.data);
            }
            
            if (eventJSON.command === "use-item") {
                useItem(eventJSON.data);
            }
            
            if (eventJSON.command === "share-item") {
                shareItem(eventJSON.data);
            }
            
            if (eventJSON.command === "web-to-script-request-nearby-users") {
                sendNearbyUsers();
            }
            
            if (eventJSON.command === "web-to-script-request-receiving-item-queue") {
                sendReceivingItemQueue();
            }
            
            if (eventJSON.command === "web-to-script-update-receiving-item-queue") {
                updateReceivingItemQueue(eventJSON.data);
            }
            
        }
    }

    tablet.webEventReceived.connect(onWebAppEventReceived);

    function sendToWeb(command, data) {
        var dataToSend = {
            "app": "inventory",
            "command": command,
            "data": data
        };
        
        tablet.emitScriptEvent(JSON.stringify(dataToSend));
    }

    function onMessageReceived(channel, message, sender, localOnly) {
        if (channel === INVENTORY_MESSAGES_CHANNEL) {
            var messageJSON = JSON.parse(message);
            // Window.alert("Passed 0 " + messageJSON.recipient + " vs " + MyAvatar.sessionUUID);
            if (messageJSON.command === "share-item" 
                && messageJSON.recipient === MyAvatar.sessionUUID) { // We are receiving an item.
                // Window.alert("Passed 1 " + messageJSON.recipient + " vs " + MyAvatar.sessionUUID);
                pushReceivedItemToQueue(sender, messageJSON.type, messageJSON.name, messageJSON.url, messageJSON.tags, messageJSON.metadata, messageJSON.version);
            }
            // Window.alert("Passed 0 " + sender + " vs " + MyAvatar.sessionUUID);
            if (messageJSON.command === "append-item" 
                && sender === MyAvatar.sessionUUID) { // We are appending an item from another of our own apps.
                appendReceivedItemToInventory(messageJSON.type, messageJSON.name, messageJSON.url, messageJSON.tags, messageJSON.metadata, messageJSON.version);
            }
        }
        // print("Message received:");
        // print("- channel: " + channel);
        // print("- message: " + message);
        // print("- sender: " + sender);
        // print("- localOnly: " + localOnly);
    }

    function sendMessage(dataToSend) {
        Messages.sendMessage(INVENTORY_MESSAGES_CHANNEL, JSON.stringify(dataToSend));
    }

    // END APP EVENT AND MESSAGING ROUTING

    // SEND AND RECEIVE INVENTORY STATE

    function receiveInventory(receivedInventoryData) {
        inventoryData = receivedInventoryData;
        saveInventory();
    }

    function sendInventory() {
        sendToWeb("script-to-web-inventory", inventoryData);
    }

    // END SEND AND RECEIVE INVENTORY STATE

    // SEND AND RECEIVE SETTINGS STATE

    function receiveSettings(receivedSettingsData) {
        inventorySettings = receivedSettingsData;
        saveSettings();
    }

    function sendSettings() {
        sendToWeb("script-to-web-settings", inventorySettings);
    }

    // END SEND AND RECEIVE SETTINGS STATE
    
    // This function bypasses the receiving item queue and goes straight into the inventory as is.
    function appendReceivedItemToInventory (type, name, url, tags, metadata, version) {
        var itemData = {
            "type": type,
            "name": name,
            "url": url,
            "tags": tags,
            "metadata": metadata,
            "version": version
        }
        
        if (isInventoryOpen) {
            sendToWeb("script-to-web-append-item", itemData);
        } else {
            appendItemWaiting = itemData;
        }
    }
    
    function appendItemWaiitngToInventory () {
        sendToWeb("script-to-web-append-item", appendItemWaiting);
    }

    function saveInventory() {
        Settings.setValue(inventoryDataSettingString, inventoryData);
    }

    function loadInventory() {
        inventoryData = Settings.getValue(inventoryDataSettingString);
    }

    function saveSettings() {
        Settings.setValue(inventorySettingsString, inventorySettings);
    }

    function loadSettings() {
        inventorySettings = Settings.getValue(inventorySettingsString);
    }

    function pushReceivedItemToQueue(senderUUID, type, name, url, tags, metadata, version) {
        console.info("Receiving an item:", name, "from:", senderUUID);
        var getAvatarData = AvatarList.getAvatar(senderUUID);
        var senderName = getAvatarData.sessionDisplayName;
        var senderDistance = Vec3.distance(MyAvatar.position, getAvatarData.position);
        
        var packageRequest = {
            "senderUUID": senderUUID,
            "senderName": senderName,
            "senderDistance": senderDistance,
            "data": {
                "type": type,
                "name": name,
                "url": url,
                "tags": tags,
                "metadata": metadata,
                "version": version
            }
        };
        
        if (receivingItemQueue.length === RECEIVING_ITEM_QUEUE_LIMIT) {
            receivingItemQueue = receivingItemQueue.slice(1, RECEIVING_ITEM_QUEUE_LIMIT);
        }
        
        receivingItemQueue.push(packageRequest);
        ui.messagesWaiting(receivingItemQueue.length > 0);
    }

    function sendReceivingItemQueue() {
        sendToWeb("script-to-web-receiving-item-queue", receivingItemQueue);
    }

    function updateReceivingItemQueue(data) {
        receivingItemQueue = data;
        ui.messagesWaiting(receivingItemQueue.length > 0);
    }

    function sendNearbyUsers() {
        var nearbyUsers = AvatarList.getAvatarsInRange(MyAvatar.position, NEARBY_USERS_SEARCH_RADIUS);
        var nearbyUsersToSend = [];
        
        nearbyUsers.forEach(function(user) {
            var objectToWrite;
            var aviDetails = AvatarList.getAvatar(user);
            var aviName = aviDetails.displayName;
            var aviDistance = Vec3.distance(MyAvatar.position, aviDetails.position);
            // Window.alert("aviName" + aviName + "user" + user + "MyAvatar.sessionUUID" + MyAvatar.sessionUUID);
            if (user !== MyAvatar.sessionUUID 
                || Controller.getValue(Controller.Hardware.Keyboard.Shift)) { // Don't add ourselves to the list!
                objectToWrite = { "name": aviName, "distance": aviDistance, "uuid": user };
                nearbyUsersToSend.push(objectToWrite);
            }        
        });

        sendToWeb("script-to-web-nearby-users", nearbyUsersToSend);
    }

    function useItem(item) {
        
        //TODO: Add animation support for avatars...?
        
        // Convert the item.type before checking it...
        item.type = item.type.toUpperCase();
        
        // Depending on the type, we decide how to load this item.
        if (item.type === "SCRIPT") {
            ScriptDiscoveryService.loadScript(item.url, true, false, false, true, false);
        }
        
        if (item.type === "MODEL") {
            Entities.addEntity({
                type: "Model",
                position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -1.5 })),
                rotation: MyAvatar.orientation,
                modelURL: item.url,
                collisionless: true
            });
        }
        
        if (item.type === "AVATAR") {
            MyAvatar.useFullAvatarURL(item.url);
        }
        
        if (item.type === "PLACE") {
            location.handleLookupString(item.url, true);
        }
        
        if (item.type === "JSON") {
            var jsonToLoad = item.url;
            if (jsonToLoad) {
                if (Clipboard.importEntities(jsonToLoad)) {
                    Clipboard.pasteEntities(
                        Vec3.sum(
                            MyAvatar.position,
                            Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -1.5 })
                        )
                    );
                }
            }
        }
        
        if (item.type === "OTHER") {
            // We don't know how to handle this yet.
            Window.alert("Item is not a known type, we are unable to use it.");
        }
    }

    function shareItem(data) {
        data.command = "share-item";
        sendMessage(data);
    }

    function initializeInventoryApp() {
        sendSettings();
        sendInventory();
        sendReceivingItemQueue();
        if (appendItemWaiting !== null) {
            sendToWeb("script-to-web-append-item", appendItemWaiting);
            appendItemWaiting = null;
        }
    }

    function onOpened() {
        isInventoryOpen = true;
    }

    function onClosed() {
        isInventoryOpen = false;
    }

    function startup() {
        
        loadInventory();
        loadSettings();
        
        Messages.messageReceived.connect(onMessageReceived);
        Messages.subscribe(INVENTORY_MESSAGES_CHANNEL);
        
        ui = new AppUi({
            buttonName: APP_NAME,
            home: Script.resolvePath("index.html"),
            graphicsDirectory: Script.resolvePath("./"), // Where your button icons are located
            onOpened: onOpened,
            onClosed: onClosed
        });
    }

    startup();

    Script.scriptEnding.connect(function () {
        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(INVENTORY_MESSAGES_CHANNEL);
    });

}()); // END LOCAL_SCOPE