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
    div.id = id;
    img.id = id + "img";
    div.style.width = "100%";
    div.style.height = "" + Math.floor(100 / imageCount) + "%";
    div.style.display = "flex";
    div.style.justifyContent = "center";
    div.style.alignItems = "center";
    div.style.position = "relative";
    if (imageCount > 1) {
        img.setAttribute("class", "multiple");
    }
    img.src = data.localPath;
    div.appendChild(img);
    document.getElementById("snapshot-images").appendChild(div);
    div.appendChild(createShareOverlay(id, img.src.split('.').pop().toLowerCase() === "gif"));
    img.onload = function () {
        var shareBar = document.getElementById(id + "shareBar");
        shareBar.style.width = img.clientWidth;
        shareBar.style.display = "inline";

        document.getElementById(id).style.height = img.clientHeight;
    }
    paths.push(data);
}
function createShareOverlay(parentID, isGif) {
    var shareOverlayContainer = document.createElement("DIV");
    shareOverlayContainer.id = parentID + "shareOverlayContainer";
    shareOverlayContainer.style.position = "absolute";
    shareOverlayContainer.style.top = "0px";
    shareOverlayContainer.style.left = "0px";
    shareOverlayContainer.style.display = "flex";
    shareOverlayContainer.style.alignItems = "flex-end";
    shareOverlayContainer.style.width = "100%";
    shareOverlayContainer.style.height = "100%";

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
            '<input type="button" class="shareButton" id="' + shareButtonID + '" onclick="selectImageToShare(' + parentID + ')" />' +
        '</div>'
    shareOverlayContainer.appendChild(shareBar);

    var shareOverlay = document.createElement("div");
    shareOverlay.id = parentID + "shareOverlay";
    shareOverlay.style.display = "none";
    shareOverlay.style.backgroundColor = "black";
    shareOverlay.style.opacity = "0.5";
    shareOverlay.style.width = "100%";
    shareOverlay.style.height = "100%";
    shareOverlayContainer.appendChild(shareOverlay);

    return shareOverlayContainer;
}
function selectImageToShare(selectedID) {
    selectedID = selectedID.id; // Why is this necessary?
    var shareOverlayContainer = document.getElementById(selectedID + "shareOverlayContainer");
    var shareBar = document.getElementById(selectedID + "shareBar");
    var shareOverlay = document.getElementById(selectedID + "shareOverlay");

    shareOverlayContainer.style.outline = "4px solid #00b4ef";
    shareOverlayContainer.style.outlineOffset = "-4px";

    shareBar.style.display = "none";

    shareOverlay.style.display = "inline";
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
