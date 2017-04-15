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

var paths = [], idCounter = 0, imageCount;
function addImage(data) {
    if (!data.localPath) {
        return;
    }
    var div = document.createElement("DIV"),
        input = document.createElement("INPUT"),
        label = document.createElement("LABEL"),
        img = document.createElement("IMG"),
        div2 = document.createElement("DIV"),
        id = "p" + idCounter++;
        img.id = id + "img";
    function toggle() { data.share = input.checked; }
    div.style.height = "" + Math.floor(100 / imageCount) + "%";
    if (imageCount > 1) {
        img.setAttribute("class", "multiple");
    }
    img.src = data.localPath;
    div.appendChild(img);
    if (imageCount > 1) { // I'd rather use css, but the included stylesheet is quite particular.
        // Our stylesheet(?) requires input.id to match label.for. Otherwise input doesn't display the check state.
        label.setAttribute('for', id); // cannot do label.for =
        input.id = id;
        input.type = "checkbox";
        input.checked = false;
        data.share = input.checked;
        input.addEventListener('change', toggle);
        div2.setAttribute("class", "property checkbox");
        div2.appendChild(input);
        div2.appendChild(label);
        div.appendChild(div2);
    } else {
        data.share = true;
    }
    document.getElementById("snapshot-images").appendChild(div);
    paths.push(data);
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
    //addImage({ localPath: 'http://lorempixel.com/1512/1680' });
    openEventBridge(function () {
        // Set up a handler for receiving the data, and tell the .js we are ready to receive it.
        EventBridge.scriptEventReceived.connect(function (message) {
            message = JSON.parse(message);
            if (message.type !== "snapshot") {
                return;
            }

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
                    document.getElementById('p0').disabled = true;
                } else {
                    var gifPath = message.action[0].localPath;
                    document.getElementById('p0').disabled = false;
                    document.getElementById('p0img').src = gifPath;
                    paths[0].localPath = gifPath;
                }
            } else {
                imageCount = message.action.length;
                message.action.forEach(addImage);
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
