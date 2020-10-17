//  checkForUpdates.js
//
//  Created by Kalila L. on 8/27/20
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

Script.include("/~/system/libraries/utils.js");
var networkingConstants = Script.require("./libraries/networkingConstants.js");

// We'll want a way to specify this later, auto detection can also be a thing, but strictly optional.
var currentBuildType = "latest";
var currentVersion = About.buildVersion; 

// Notification Variables
var NOTIFICATION_CHANNEL = "System-Notifications";
var NOTIFICATION_TYPE = "Update-Notification";
var NOTIFICATION_CATEGORY = "Interface";
var notificationText = "An update is available: ";
var notificationColor = { red: 137, green: 63, blue: 255 };

function retrieveMetadata() {
    var requireMetadata = Script.require(networkingConstants.INTERFACE_METADATA_SOURCE + '?' + Date.now());
    
    if (requireMetadata) {
        return requireMetadata;
    } else {
        return false;
    }
}

function checkForUpdates() {
    var vircadiaMetadata = retrieveMetadata();
    
    // Don't check for updates on a dev build.
    if (vircadiaMetadata && currentVersion !== "dev") {
        var checkVersion = semanticVersionCompare(currentVersion, vircadiaMetadata[currentBuildType].version);
        
        // An update is available!
        if (checkVersion === -1) {
            Messages.sendMessage(NOTIFICATION_CHANNEL, JSON.stringify({
                type: NOTIFICATION_TYPE,
                category: NOTIFICATION_CATEGORY,
                channel: "Local",
                position: MyAvatar.position,
                colour: notificationColor,
                message: notificationText + vircadiaMetadata[currentBuildType].version,
            }));
        }
    }
}

checkForUpdates();
