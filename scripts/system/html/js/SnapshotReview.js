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
    img.id = id + "img";
    div.style.width = "100%";
    div.style.height = "" + Math.floor(100 / imageCount) + "%";
    div.style.display = "flex";
    div.style.justifyContent = "center";
    div.style.alignItems = "center";
    div.style.marginBottom = "5px";
    div.style.position = "relative";
    if (imageCount > 1) {
        img.setAttribute("class", "multiple");
    }
    img.src = data.localPath;
    div.appendChild(img);
    document.getElementById("snapshot-images").appendChild(div);
    div.appendChild(createShareOverlayDiv(id, img.src.split('.').pop().toLowerCase() === "gif"));
    img.onload = function () {
        var shareBar = document.getElementById(id + "shareBar");
        shareBar.style.width = img.clientWidth;
        shareBar.style.display = "inline";
    }
    paths.push(data);
}
function createShareOverlayDiv(parentID, isGif) {
    var div = document.createElement("DIV");
    div.style.position = "absolute";
    div.style.display = "flex";
    div.style.alignItems = "flex-end";
    div.style.top = "0px";
    div.style.left = "0px";
    div.style.width = "100%";
    div.style.height = "100%";

    var shareBar = document.createElement("div");
    shareBar.id = parentID + "shareBar"
    shareBar.style.display = "none";
    shareBar.style.width = "100%";
    shareBar.style.height = "60px";
    shareBar.style.lineHeight = "60px";
    shareBar.style.clear = "both";
    shareBar.style.marginLeft = "auto";
    shareBar.style.marginRight = "auto";
    shareBar.innerHTML = isGif ? '<span class="gifLabel">GIF</span>' : "";
    var shareButtonID = parentID + "shareButton";
    shareBar.innerHTML += '<div class="shareButtonDiv">' +
            '<label class="shareButtonLabel" for="' + shareButtonID + '">SHARE</label>' +
            '<input type="button" class="shareButton" id="' + shareButtonID + '" onclick="" />' +
        '</div>'
    div.appendChild(shareBar);

    var shareOverlay = document.createElement("div");
    shareOverlay.style.visibilty = "hidden";
    shareOverlay.style.backgroundColor = "black";
    shareOverlay.style.opacity = "0.5";
    div.appendChild(shareOverlay);

    return div;
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
    // TESTING FUNCTIONS START
    // Uncomment and modify the lines below to test SnapshotReview in a browser.
    imageCount = 2;
    addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/2017-01-27 50 Avatars!/!!!.gif' });
    addImage({ localPath: 'http://lorempixel.com/553/255' });
    //addImage({localPath: 'c:/Users/howar/OneDrive/Pictures/hifi-snap-by--on-2016-07-27_12-58-43.jpg'});
    // TESTING FUNCTIONS END

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
                            document.getElementById('p0img').src = gifPath;
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
