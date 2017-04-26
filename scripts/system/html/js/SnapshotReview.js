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

var paths = [];
var idCounter = 0;
var imageCount = 0;
function showSetupInstructions() {
    var snapshotImagesDiv = document.getElementById("snapshot-images");
    snapshotImagesDiv.className = "snapshotInstructions";
    snapshotImagesDiv.innerHTML = '<img class="centeredImage" src="./img/snapshotIcon.png" alt="Snapshot Instructions" width="64" height="64"/>' +
    '<br/>' +
    '<p>This app lets you take and share snaps and GIFs with your connections in High Fidelity.</p>' +
    "<h4>Setup Instructions</h4>" +
    "<p>Before you can begin taking snaps, please choose where you'd like to save snaps on your computer:</p>" +
    '<br/>' +
    '<input type="button" value="CHOOSE" onclick="chooseSnapshotLocation()" />';
    document.getElementById("snap-button").disabled = true;
}
function chooseSnapshotLocation() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "chooseSnapshotLocation"
    }));
}
function clearImages() {
    document.getElementById("snap-button").disabled = false;
    var snapshotImagesDiv = document.getElementById("snapshot-images");
    snapshotImagesDiv.classList.remove("snapshotInstructions");
    while (snapshotImagesDiv.hasChildNodes()) {
        snapshotImagesDiv.removeChild(snapshotImagesDiv.lastChild);
    }
    paths = [];
    imageCount = 0;
    idCounter = 0;
}
function addImage(image_data, isGifLoading, isShowingPreviousImages, canSharePreviousImages, hifiShareButtonsDisabled) {
    if (!image_data.localPath) {
        return;
    }
    var div = document.createElement("DIV");
    var id = "p" + idCounter++;
    var img = document.createElement("IMG");
    div.id = id;
    img.id = id + "img";
    div.style.width = "100%";
    div.style.height = "" + 502 / imageCount + "px";
    div.style.display = "flex";
    div.style.justifyContent = "center";
    div.style.alignItems = "center";
    div.style.position = "relative";
    if (imageCount > 1) {
        img.setAttribute("class", "multiple");
    }
    img.src = image_data.localPath;
    div.appendChild(img);
    document.getElementById("snapshot-images").appendChild(div);
    var isGif = img.src.split('.').pop().toLowerCase() === "gif";
    paths.push(image_data.localPath);
    if (!isGifLoading && !isShowingPreviousImages) {
        shareForUrl(id);
    } else if (isShowingPreviousImages && canSharePreviousImages) {
        appendShareBar(id, image_data.story_id, isGif, hifiShareButtonsDisabled)
    }
}
function appendShareBar(divID, story_id, isGif, hifiShareButtonsDisabled) {
    var story_url = "https://highfidelity.com/user_stories/" + story_id;
    var parentDiv = document.getElementById(divID);
    parentDiv.setAttribute('data-story-id', story_id);
    document.getElementById(divID).appendChild(createShareOverlay(divID, isGif, story_url, hifiShareButtonsDisabled));
}
function createShareOverlay(parentID, isGif, shareURL, hifiShareButtonsDisabled) {
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
    shareBar.style.display = "inline";
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

    var shareOverlayBackground = document.createElement("div");
    shareOverlayBackground.id = parentID + "shareOverlayBackground";
    shareOverlayBackground.style.display = "none";
    shareOverlayBackground.style.position = "absolute";
    shareOverlayBackground.style.zIndex = "1";
    shareOverlayBackground.style.top = "0px";
    shareOverlayBackground.style.left = "0px";
    shareOverlayBackground.style.backgroundColor = "black";
    shareOverlayBackground.style.opacity = "0.5";
    shareOverlayBackground.style.width = "100%";
    shareOverlayBackground.style.height = "100%";
    shareOverlayContainer.appendChild(shareOverlayBackground);

    var shareOverlay = document.createElement("div");
    shareOverlay.id = parentID + "shareOverlay";
    shareOverlay.className = "shareOverlayDiv";
    shareOverlay.style.display = "none";
    shareOverlay.style.width = "100%";
    shareOverlay.style.height = "100%";
    shareOverlay.style.zIndex = "2";
    var shareWithEveryoneButtonID = parentID + "shareWithEveryoneButton";
    var inviteConnectionsCheckboxID = parentID + "inviteConnectionsCheckbox";
    var facebookButtonID = parentID + "facebookButton";
    var twitterButtonID = parentID + "twitterButton";
    shareOverlay.innerHTML = '<label class="shareOverlayLabel">SHARE</label>' +
        '<br/>' +
        '<div class="shareControls">' +
            '<div class="hifiShareControls">' +
                '<input type="button"' + (hifiShareButtonsDisabled ? ' disabled="disabled"' : '') + ' class="shareWithEveryone" id="' + shareWithEveryoneButtonID + '" value="SHARE WITH EVERYONE" onclick="shareWithEveryone(' + parentID + ', ' + isGif + ')" /><br>' +
                '<input type="checkbox"' + (hifiShareButtonsDisabled ? ' disabled="disabled"' : '') + ' class="inviteConnections" id="' + inviteConnectionsCheckboxID + '" checked="checked" />' +
                '<label class="shareButtonLabel" for="' + inviteConnectionsCheckboxID + '">Invite My Connections</label><br>' +
                '<input type="button" value="CANCEL" onclick="cancelSharing(' + parentID + ')" />' +
            '</div>' +
            '<div class="buttonShareControls">' +
                '<a class="facebookButton" id="' + facebookButtonID + '" onClick="shareButtonClicked(' + parentID + ')" target="_blank" href="https://www.facebook.com/dialog/feed?app_id=1585088821786423&link=' + shareURL + '"></a>' +
                '<a class="twitterButton" id="' + twitterButtonID + '" onClick="shareButtonClicked(' + parentID + ')" target="_blank" href="https://twitter.com/intent/tweet?text=I%20just%20took%20a%20snapshot!&url=' + shareURL + '&via=highfidelity&hashtags=VR,HiFi"></a>' +
            '</div>' +
        '</div>';
    shareOverlayContainer.appendChild(shareOverlay);

    return shareOverlayContainer;
}
function selectImageToShare(selectedID) {
    selectedID = selectedID.id; // `selectedID` is passed as an HTML object to these functions; we just want the ID
    var shareOverlayContainer = document.getElementById(selectedID + "shareOverlayContainer");
    var shareBar = document.getElementById(selectedID + "shareBar");
    var shareOverlayBackground = document.getElementById(selectedID + "shareOverlayBackground");
    var shareOverlay = document.getElementById(selectedID + "shareOverlay");

    shareOverlay.style.outline = "4px solid #00b4ef";
    shareOverlay.style.outlineOffset = "-4px";

    shareBar.style.display = "none";

    shareOverlayBackground.style.display = "inline";
    shareOverlay.style.display = "inline";
}
function shareForUrl(selectedID) {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "shareSnapshotForUrl",
        data: paths[parseInt(selectedID.substring(1))]
    }));
}
function shareWithEveryone(selectedID, isGif) {
    selectedID = selectedID.id; // `selectedID` is passed as an HTML object to these functions; we just want the ID

    document.getElementById(selectedID + "shareWithEveryoneButton").setAttribute("disabled", "disabled");
    document.getElementById(selectedID + "inviteConnectionsCheckbox").setAttribute("disabled", "disabled");

    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "shareSnapshotWithEveryone",
        story_id: document.getElementById(selectedID).getAttribute("data-story-id"),
        isAnnouncement: document.getElementById(selectedID + "inviteConnectionsCheckbox").getAttribute("checked"),
        isGif: isGif
    }));
}
function shareButtonClicked(selectedID) {
    selectedID = selectedID.id; // `selectedID` is passed as an HTML object to these functions; we just want the ID
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "shareButtonClicked",
        story_id: document.getElementById(selectedID).getAttribute("data-story-id")
    }));
}
function cancelSharing(selectedID) {
    selectedID = selectedID.id; // `selectedID` is passed as an HTML object to these functions; we just want the ID
    var shareOverlayContainer = document.getElementById(selectedID + "shareOverlayContainer");
    var shareBar = document.getElementById(selectedID + "shareBar");
    var shareOverlayBackground = document.getElementById(selectedID + "shareOverlayBackground");
    var shareOverlay = document.getElementById(selectedID + "shareOverlay");

    shareOverlay.style.outline = "none";

    shareBar.style.display = "inline";

    shareOverlayBackground.style.display = "none";
    shareOverlay.style.display = "none";
}

function handleCaptureSetting(setting) {
    var stillAndGif = document.getElementById('stillAndGif');
    var stillOnly = document.getElementById('stillOnly');
    stillAndGif.checked = setting;
    stillOnly.checked = !setting;

    stillAndGif.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "captureStillAndGif"
        }));
    }
    stillOnly.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "captureStillOnly"
        }));
    }

}
window.onload = function () {
    testInBrowser(false);
    openEventBridge(function () {
        // Set up a handler for receiving the data, and tell the .js we are ready to receive it.
        EventBridge.scriptEventReceived.connect(function (message) {

            message = JSON.parse(message);

            if (message.type !== "snapshot") {
                return;
            }
            
            switch (message.action) {
                case 'showSetupInstructions':
                    showSetupInstructions();
                    break;
                case 'snapshotLocationChosen':
                    clearImages();
                    break;
                case 'clearPreviousImages':
                    clearImages();
                    break;
                case 'showPreviousImages':
                    clearImages();
                    var messageOptions = message.options;
                    imageCount = message.image_data.length;
                    message.image_data.forEach(function (element, idx, array) {
                        addImage(element, true, true, message.canShare, message.image_data[idx].buttonDisabled);
                    });
                    break;
                case 'addImages':
                    // The last element of the message contents list contains a bunch of options,
                    // including whether or not we can share stuff
                    // The other elements of the list contain image paths.
                    var messageOptions = message.options;

                    if (messageOptions.containsGif) {
                        if (messageOptions.processingGif) {
                            imageCount = message.image_data.length + 1; // "+1" for the GIF that'll finish processing soon
                            message.image_data.unshift({ localPath: messageOptions.loadingGifPath });
                            message.image_data.forEach(function (element, idx, array) {
                                addImage(element, idx === 0, false, false);
                            });
                        } else {
                            var gifPath = message.image_data[0].localPath;
                            var p0img = document.getElementById('p0img');
                            p0img.src = gifPath;

                            paths[0] = gifPath;
                            shareForUrl("p0");
                        }
                    } else {
                        imageCount = message.image_data.length;
                        message.image_data.forEach(function (element, idx, array) {
                            addImage(element, false, false, false);
                        });
                    }
                    break;
                case 'captureSettings':
                    handleCaptureSetting(message.setting);
                    break;
                case 'snapshotUploadComplete':
                    var isGif = message.image_url.split('.').pop().toLowerCase() === "gif";
                    appendShareBar(isGif || imageCount === 1 ? "p0" : "p1", message.story_id, isGif);
                    break;
                default:
                    console.log("Unknown message action received in SnapshotReview.js.");
                    break;
            }
        });

        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "ready"
        }));
    });;
};
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

function testInBrowser(isTestingSetupInstructions) {
    if (isTestingSetupInstructions) {
        showSetupInstructions();
    } else {
        imageCount = 1;
        //addImage({ localPath: 'http://lorempixel.com/553/255' });
        addImage({ localPath: 'C:/Users/valef/Desktop/hifi-snap-by-zfox-on-2017-04-26_10-26-53.jpg' }, false, true, true, false);
    }
}
