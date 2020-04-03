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

// APP EVENT ROUTING

function onWebAppEventReceived(event) {
    var eventJSON = JSON.parse(event);
    if (eventJSON.app == "inventory") { // This is our web app!
        print("gemstoneApp.js received a web event: " + event);
    }
}

tablet.webEventReceived.connect(onWebAppEventReceived);

// END APP EVENT ROUTING

function saveInventory() {
    
}

function loadInventory() {
    
}

function receivingItem() {
    
}

function shareItem() {
    
}

function onOpened() {
    console.log("hello world!");
}

function onClosed() {
    console.log("hello world!");
}

function startup() {
    ui = new AppUi({
        buttonName: "INVENTORY",
        home: Script.resolvePath("inventory.html"),
        graphicsDirectory: Script.resolvePath("./"), // Where your button icons are located
        onOpened: onOpened,
        onClosed: onClosed
    });
}
startup();
}()); // END LOCAL_SCOPE