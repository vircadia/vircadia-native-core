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
    '<div style="text-align:center;">' +
        '<input class="blueButton" style="margin-left:auto;margin-right:auto;width:130px;" type="button" value="CHOOSE" onclick="chooseSnapshotLocation()" />' +
    '</div>';
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
function addImage(image_data, isGifLoading, canShare, isShowingPreviousImages, blastButtonDisabled, hifiButtonDisabled) {
    if (!image_data.localPath) {
        return;
    }
    var id = "p" + idCounter++;
    // imageContainer setup
    var imageContainer = document.createElement("DIV");
    imageContainer.id = id;
    imageContainer.style.width = "95%";
    imageContainer.style.height = "240px";
    imageContainer.style.margin = "5px auto";
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
    if (!isGifLoading && !isShowingPreviousImages && canShare) {
        shareForUrl(id);
    } else if (isShowingPreviousImages && canShare && image_data.story_id) {
        appendShareBar(id, image_data.story_id, isGif, blastButtonDisabled, hifiButtonDisabled)
    }
}
function appendShareBar(divID, story_id, isGif, blastButtonDisabled, hifiButtonDisabled) {
    var story_url = "https://highfidelity.com/user_stories/" + story_id;
    var parentDiv = document.getElementById(divID);
    parentDiv.setAttribute('data-story-id', story_id);
    document.getElementById(divID).appendChild(createShareBar(divID, isGif, story_url, blastButtonDisabled, hifiButtonDisabled));
    if (divID === "p0") {
        selectImageToShare(divID, true);
    }
}
function createShareBar(parentID, isGif, shareURL, blastButtonDisabled, hifiButtonDisabled) {
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
        '<div class="shareButtons" id="' + shareButtonsDivID + '" style="visibility:hidden">' +
            '<input type="button"' + (blastButtonDisabled ? ' disabled' : '') + ' class="blastToConnections blueButton" id="' + blastToConnectionsButtonID + '" value="BLAST TO MY CONNECTIONS" onclick="blastToConnections(' + parentID + ', ' + isGif + ')" />' +
            '<input type="button"' + (hifiButtonDisabled ? ' disabled' : '') + ' class="shareWithEveryone" id="' + shareWithEveryoneButtonID + '" onclick="shareWithEveryone(' + parentID + ', ' + isGif + ')" />' +
            '<a class="facebookButton" id="' + facebookButtonID + '" onclick="shareButtonClicked(' + parentID + ')" target="_blank" href="https://www.facebook.com/dialog/feed?app_id=1585088821786423&link=' + shareURL + '"></a>' +
            '<a class="twitterButton" id="' + twitterButtonID + '" onclick="shareButtonClicked(' + parentID + ')" target="_blank" href="https://twitter.com/intent/tweet?text=I%20just%20took%20a%20snapshot!&url=' + shareURL + '&via=highfidelity&hashtags=VR,HiFi"></a>' +
        '</div>' +
        '<div class="showShareButtonsButtonDiv" id="' + showShareButtonsButtonDivID + '">' +
            '<label id="' + showShareButtonsLabelID + '" for="' + showShareButtonsButtonID + '">SHARE</label>' +
            '<input type="button" class="showShareButton inactive" id="' + showShareButtonsButtonID + '" onclick="selectImageToShare(' + parentID + ', true)" />' +
            '<div class="showShareButtonDots">' +
                '&#xe019;' +
            '</div>' +
        '</div>';

    // Add onclick handler to parent DIV's img to toggle share buttons
    document.getElementById(parentID + 'img').onclick = function () { selectImageToShare(parentID, true) };

    return shareBar;
}
function selectImageToShare(selectedID, isSelected) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var imageContainer = document.getElementById(selectedID);
    var image = document.getElementById(selectedID + 'img');
    var shareBar = document.getElementById(selectedID + "shareBar");
    var shareButtonsDiv = document.getElementById(selectedID + "shareButtonsDiv");
    var showShareButtonsButton = document.getElementById(selectedID + "showShareButtonsButton");

    if (isSelected) {
        showShareButtonsButton.onclick = function () { selectImageToShare(selectedID, false) };
        showShareButtonsButton.classList.remove("inactive");
        showShareButtonsButton.classList.add("active");

        image.onclick = function () { selectImageToShare(selectedID, false) };
        imageContainer.style.outline = "4px solid #00b4ef";
        imageContainer.style.outlineOffset = "-4px";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.5)";

        shareButtonsDiv.style.visibility = "visible";

        var containers = document.getElementsByClassName("shareControls");
        var parentID;
        for (var i = 0; i < containers.length; ++i) {
            parentID = containers[i].id.slice(0, 2);
            if (parentID !== selectedID) {
                selectImageToShare(parentID, false);
            }
        }
    } else {
        showShareButtonsButton.onclick = function () { selectImageToShare(selectedID, true) };
        showShareButtonsButton.classList.remove("active");
        showShareButtonsButton.classList.add("inactive");

        image.onclick = function () { selectImageToShare(selectedID, true) };
        imageContainer.style.outline = "none";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.0)";

        shareButtonsDiv.style.visibility = "hidden";
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

    document.getElementById(selectedID + "blastToConnectionsButton").disabled = true;

    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "blastToConnections",
        story_id: document.getElementById(selectedID).getAttribute("data-story-id"),
        isGif: isGif
    }));
}
function shareWithEveryone(selectedID, isGif) {
    selectedID = selectedID.id; // `selectedID` is passed as an HTML object to these functions; we just want the ID

    document.getElementById(selectedID + "shareWithEveryoneButton").disabled = true;

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
    // Uncomment the line below to test functionality in a browser.
    // See definition of "testInBrowser()" to modify tests.
    //testInBrowser(false);
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
                        addImage(element, true, message.canShare, true, message.image_data[idx].blastButtonDisabled, message.image_data[idx].hifiButtonDisabled);
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
                            message.image_data.push({ localPath: messageOptions.loadingGifPath });
                            message.image_data.forEach(function (element, idx, array) {
                                addImage(element, idx === 1, idx === 0 && messageOptions.canShare, false);
                            });
                        } else {
                            var gifPath = message.image_data[0].localPath;
                            var p1img = document.getElementById('p1img');
                            p1img.src = gifPath;

                            paths[1] = gifPath;
                            if (messageOptions.canShare) {
                                shareForUrl("p1");
                            }
                        }
                    } else {
                        imageCount = message.image_data.length;
                        message.image_data.forEach(function (element, idx, array) {
                            addImage(element, false, messageOptions.canShare, false);
                        });
                    }
                    break;
                case 'captureSettings':
                    handleCaptureSetting(message.setting);
                    break;
                case 'snapshotUploadComplete':
                    var isGif = message.image_url.split('.').pop().toLowerCase() === "gif";
                    appendShareBar(isGif ? "p1" : "p0", message.story_id, isGif);
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
        addImage({ localPath: 'C:/Users/valef/Desktop/hifi-snap-by-zfox-on-2017-05-01_15-48-15.gif' }, false, true, true, false, false);
        addImage({ localPath: 'C:/Users/valef/Desktop/hifi-snap-by-zfox-on-2017-05-01_15-48-15.jpg' }, false, true, true, false, false);
    }
}
