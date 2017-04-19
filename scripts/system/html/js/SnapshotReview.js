"use strict";
//
//  SnapshotReview.js
//  scripts/system/html/js/
//
//  Created by Howard Stearns 8/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var paths = [], idCounter = 0, imageCount = 1;
function addImage(data) {
    if (!data.localPath) {
        return;
    }
    var div = document.createElement("DIV");
    var id = "p" + idCounter++;
    var img = document.createElement("IMG");
    img.id = "img" + id;
    div.style.height = "" + Math.floor(100 / imageCount) + "%";
    if (imageCount > 1) {
        img.setAttribute("class", "multiple");
    }
    img.src = data.localPath;
    div.appendChild(img);
    document.getElementById("snapshot-images").appendChild(div);
    paths.push(data);
}
function handleCaptureSetting(setting) {
    var stillAndGif = document.getElementById('stillAndGif');
    var stillOnly = document.getElementById('stillOnly');
    stillAndGif.checked = setting;
    stillOnly.checked = !setting;

    stillAndGif.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "captureSettings",
            action: true
        }));
    }
    stillOnly.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "captureSettings",
            action: false
        }));
    }

}
function handleShareButtons(messageOptions) {
    var openFeed = document.getElementById('openFeed');
    openFeed.checked = messageOptions.openFeedAfterShare;
    openFeed.onchange = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: (openFeed.checked ? "setOpenFeedTrue" : "setOpenFeedFalse")
        }));
    };

    if (!messageOptions.canShare) {
        // this means you may or may not be logged in, but can't share
        // because you are not in a public place.
        document.getElementById("sharing").innerHTML = "<p class='prompt'>Snapshots can be shared when they're taken in shareable places.";
    }
}
window.onload = function () {
    // Something like the following will allow testing in a browser.
    //addImage({localPath: 'c:/Users/howar/OneDrive/Pictures/hifi-snap-by--on-2016-07-27_12-58-43.jpg'});
    addImage({ localPath: 'http://lorempixel.com/1512/1680' });
    openEventBridge(function () {
        // Set up a handler for receiving the data, and tell the .js we are ready to receive it.
        EventBridge.scriptEventReceived.connect(function (message) {

            message = JSON.parse(message);

            switch (message.type) {
                case 'snapshot':
                    // The last element of the message contents list contains a bunch of options,
                    // including whether or not we can share stuff
                    // The other elements of the list contain image paths.
                    var messageOptions = message.action.pop();
                    handleShareButtons(messageOptions);

                    if (messageOptions.containsGif) {
                        if (messageOptions.processingGif) {
                            imageCount = message.action.length + 1; // "+1" for the GIF that'll finish processing soon
                            message.action.unshift({ localPath: messageOptions.loadingGifPath });
                            message.action.forEach(addImage);
                        } else {
                            var gifPath = message.action[0].localPath;
                            document.getElementById('imgp0').src = gifPath;
                            paths[0].localPath = gifPath;
                        }
                    } else {
                        imageCount = message.action.length;
                        message.action.forEach(addImage);
                    }
                    break;
                case 'snapshotSettings':
                    handleCaptureSetting(message.action);
                    break;
                default:
                    return;
            }
        });
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "ready"
        }));
    });

};
// beware of bug: Cannot send objects at top level. (Nested in arrays is fine.)
function shareSelected() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: paths
    }));
}
function doNotShare() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: []
    }));
}
function snapshotSettings() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "openSettings"
    }));
}
function takeSnapshot() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "takeSnapshot"
    }));
}
