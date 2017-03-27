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

var paths = [], idCounter = 0, useCheckboxes;
function addImage(data) {
    if (!data.localPath) {
        return;
    }
    var div = document.createElement("DIV"),
        input = document.createElement("INPUT"),
        label = document.createElement("LABEL"),
        img = document.createElement("IMG"),
        id = "p" + idCounter++;
    function toggle() { data.share = input.checked; }
    img.src = data.localPath;
    div.appendChild(img);
    if (useCheckboxes) { // I'd rather use css, but the included stylesheet is quite particular.
        // Our stylesheet(?) requires input.id to match label.for. Otherwise input doesn't display the check state.
        label.setAttribute('for', id); // cannot do label.for =
        input.id = id;
        input.type = "checkbox";
        input.checked = (id === "p0");
        data.share = input.checked;
        input.addEventListener('change', toggle);
        div.class = "property checkbox";
        div.appendChild(input);
        div.appendChild(label);
    } else {
        data.share = true;
    }
    document.getElementById("snapshot-images").appendChild(div);
    paths.push(data);
}
function handleShareButtons(shareMsg) {
    var openFeed = document.getElementById('openFeed');
    openFeed.checked = shareMsg.openFeedAfterShare;
    openFeed.onchange = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: (openFeed.checked ? "setOpenFeedTrue" : "setOpenFeedFalse")
        }));
    };

    if (!shareMsg.canShare) {
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

            // last element of list contains a bool for whether or not we can share stuff
            var shareMsg = message.action.pop();
            handleShareButtons(shareMsg);
            
            // rest are image paths which we add
            useCheckboxes = message.action.length > 1;
            message.action.forEach(addImage);
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
