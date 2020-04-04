//
//  inventory.js
//
//  Created by kasenvr@gmail.com on 2 Mar 2020
//  Copyright 2020 Vircadia Contributors
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () { // BEGIN LOCAL_SCOPE
var AppUi = Script.require('appUi');
var ui;
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

var inventoryDataSettingString = "inventoryApp.data";
var inventoryData;

// APP EVENT AND MESSAGING ROUTING

function onWebAppEventReceived(event) {
    var eventJSON = JSON.parse(event);
    if (eventJSON.app == "inventory") { // This is our web app!
        print("inventory.js received a web event: " + event);
        
        if (eventJSON.command == "ready") {
            initializeInventoryApp();
        }
        
        if (eventJSON.command == "web-to-script-inventory") {
            receiveInventory(eventJSON.data);
        }
        
    }
}

tablet.webEventReceived.connect(onWebAppEventReceived);

function sendToWeb(command, data) {
    var dataToSend = {
        "app": "inventory",
        "command": command,
        "data": data
    }
    tablet.emitScriptEvent(dataToSend);
}

// var inventoryMessagesChannel = "com.vircadia.inventory";

// function onMessageReceived(channel, message, sender, localOnly) {
//     if (channel == inventoryMessagesChannel) {
//         var messageJSON = JSON.parse(message);
//     }
//     print("Message received:");
//     print("- channel: " + channel);
//     print("- message: " + message);
//     print("- sender: " + sender);
//     print("- localOnly: " + localOnly);
// }

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

function saveInventory() {
    Settings.setValue(inventoryDataSettingString, inventoryData);
}

function loadInventory() {
    inventoryData = Settings.getValue(inventoryDataSettingString);
}

function receivingItem() {
    
}

function shareItem() {
    
}

function initializeInventoryApp() {
    sendInventory();
}

function onOpened() {
    console.log("hello world!");
}

function onClosed() {
    console.log("hello world!");
}

function startup() {
    
    loadInventory();
    
    ui = new AppUi({
        buttonName: "INVENTORY",
        home: Script.resolvePath("inventory.html"),
        graphicsDirectory: Script.resolvePath("./"), // Where your button icons are located
        onOpened: onOpened,
        onClosed: onClosed
    });
}
startup();

Script.scriptEnding.connect(function () {
    // Messages.messageReceived.disconnect(onMessageReceived);
    // Messages.unsubscribe(inventoryMessagesChannel);
});

}()); // END LOCAL_SCOPE