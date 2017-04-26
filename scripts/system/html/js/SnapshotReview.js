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
function showSetupComplete() {
    var snapshotImagesDiv = document.getElementById("snapshot-images");
    snapshotImagesDiv.className = "snapshotInstructions";
    snapshotImagesDiv.innerHTML = '<img class="centeredImage" src="./img/snapshotIcon.png" alt="Snapshot Instructions" width="64" height="64"/>' +
    '<br/>' +
    "<h4>You're all set!</h4>" +
    '<p>Try taking a snapshot by pressing the red button below.</p>';
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
    var id = "p" + idCounter++;
    // imageContainer setup
    var imageContainer = document.createElement("DIV");
    imageContainer.id = id;
    imageContainer.style.width = "100%";
    imageContainer.style.height = "251px";
    imageContainer.style.display = "flex";
    imageContainer.style.justifyContent = "center";
    imageContainer.style.alignItems = "center";
    imageContainer.style.position = "relative";
    // img setup
    var img = document.createElement("IMG");
    img.id = id + "img";
    if (imageCount > 1) {
        img.setAttribute("class", "multiple");
    }
    img.src = image_data.localPath;
    imageContainer.appendChild(img);
    document.getElementById("snapshot-images").appendChild(imageContainer);
    paths.push(image_data.localPath);
    var isGif = img.src.split('.').pop().toLowerCase() === "gif";
    if (isGif) {
        imageContainer.innerHTML += '<span class="gifLabel">GIF</span>';
    }
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
    document.getElementById(divID).appendChild(createShareBar(divID, isGif, story_url, hifiShareButtonsDisabled));
}
function createShareBar(parentID, isGif, shareURL, hifiShareButtonsDisabled) {
    var shareBar = document.createElement("div");
    shareBar.id = parentID + "shareBar";
    shareBar.className = "shareControls";
    var shareButtonsDivID = parentID + "shareButtonsDiv";
    var showShareButtonsButtonDivID = parentID + "showShareButtonsButtonDiv";
    var showShareButtonsButtonID = parentID + "showShareButtonsButton";
    var showShareButtonsLabelID = parentID + "showShareButtonsLabel";
    var blastToConnectionsButtonID = parentID + "blastToConnectionsButton";
    var shareWithEveryoneButtonID = parentID + "shareWithEveryoneButton";
    var facebookButtonID = parentID + "facebookButton";
    var twitterButtonID = parentID + "twitterButton";
    shareBar.innerHTML += '' +
        '<div class="shareButtons" id="' + shareButtonsDivID + '" style="opacity:0">' +
            '<input type="button"' + (hifiShareButtonsDisabled ? ' disabled="disabled"' : '') + ' class="blastToConnections blueButton" id="' + blastToConnectionsButtonID + '" value="BLAST TO MY CONNECTIONS" onclick="blastToConnections(' + parentID + ', ' + isGif + ')" />' +
            '<input type="button"' + (hifiShareButtonsDisabled ? ' disabled="disabled"' : '') + ' class="shareWithEveryone" id="' + shareWithEveryoneButtonID + '" onclick="shareWithEveryone(' + parentID + ', ' + isGif + ')" />' +
            '<a class="facebookButton" id="' + facebookButtonID + '" onClick="shareButtonClicked(' + parentID + ')" target="_blank" href="https://www.facebook.com/dialog/feed?app_id=1585088821786423&link=' + shareURL + '"></a>' +
            '<a class="twitterButton" id="' + twitterButtonID + '" onClick="shareButtonClicked(' + parentID + ')" target="_blank" href="https://twitter.com/intent/tweet?text=I%20just%20took%20a%20snapshot!&url=' + shareURL + '&via=highfidelity&hashtags=VR,HiFi"></a>' +
        '</div>' +
        '<div class="showShareButtonsButtonDiv" id="' + showShareButtonsButtonDivID + '">' +
            '<label id="' + showShareButtonsLabelID + '" for="' + showShareButtonsButtonID + '">SHARE</label>' +
            '<input type="button" class="showShareButton inactive" id="' + showShareButtonsButtonID + '" onclick="selectImageToShare(' + parentID + ', true)" />' +
            '<div class="showShareButtonDots">' +
                '<span></span><span></span><span></span>' +
            '</div>' +
        '</div>';

    return shareBar;
}
function selectImageToShare(selectedID, isSelected) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var imageContainer = document.getElementById(selectedID);
    var shareBar = document.getElementById(selectedID + "shareBar");
    var shareButtonsDiv = document.getElementById(selectedID + "shareButtonsDiv");
    var showShareButtonsButton = document.getElementById(selectedID + "showShareButtonsButton");

    if (isSelected) {
        showShareButtonsButton.onclick = function () { selectImageToShare(selectedID, false) };
        showShareButtonsButton.classList.remove("inactive");
        showShareButtonsButton.classList.add("active");

        imageContainer.style.outline = "4px solid #00b4ef";
        imageContainer.style.outlineOffset = "-4px";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.5)";

        shareButtonsDiv.style.opacity = "1.0";
    } else {
        showShareButtonsButton.onclick = function () { selectImageToShare(selectedID, true) };
        showShareButtonsButton.classList.remove("active");
        showShareButtonsButton.classList.add("inactive");

        imageContainer.style.outline = "none";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.0)";

        shareButtonsDiv.style.opacity = "0.0";
    }
}
function shareForUrl(selectedID) {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "shareSnapshotForUrl",
        data: paths[parseInt(selectedID.substring(1))]
    }));
}
function blastToConnections(selectedID, isGif) {
    selectedID = selectedID.id; // `selectedID` is passed as an HTML object to these functions; we just want the ID

    document.getElementById(selectedID + "blastToConnectionsButton").setAttribute("disabled", "disabled");
    document.getElementById(selectedID + "shareWithEveryoneButton").setAttribute("disabled", "disabled");

    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "blastToConnections",
        story_id: document.getElementById(selectedID).getAttribute("data-story-id"),
        isGif: isGif
    }));
}
function shareWithEveryone(selectedID, isGif) {
    selectedID = selectedID.id; // `selectedID` is passed as an HTML object to these functions; we just want the ID

    document.getElementById(selectedID + "blastToConnectionsButton").setAttribute("disabled", "disabled");
    document.getElementById(selectedID + "shareWithEveryoneButton").setAttribute("disabled", "disabled");

    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "shareSnapshotWithEveryone",
        story_id: document.getElementById(selectedID).getAttribute("data-story-id"),
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
    var shareBar = document.getElementById(selectedID + "shareBar");

    shareBar.style.display = "inline";
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
                    showSetupComplete();
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
        addImage({ localPath: 'C:/Users/valef/Desktop/hifi-snap-by-zfox-on-2017-04-26_10-26-53.gif' }, false, true, true, false);
    }
}
