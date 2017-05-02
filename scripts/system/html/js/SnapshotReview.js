/*jslint browser:true */
/*jslint maxlen: 180*/
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
        '<p>Take and share snaps and GIFs with people in High Fidelity, Facebook, and Twitter.</p>' +
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
        '<div style="text-align:center;font-weight:bold;">' +
            '<p>Snapshot location set.</p>' +
            '<p>Press the big red button to take a snap!</p>' +
        '</div>';
}
function showSnapshotInstructions() {
    var snapshotImagesDiv = document.getElementById("snapshot-images");
    snapshotImagesDiv.className = "snapshotInstructions";
    snapshotImagesDiv.innerHTML = '<img class="centeredImage" src="./img/snapshotIcon.png" alt="Snapshot Instructions" width="64" height="64"/>' +
        '<br/>' +
        '<p>Take and share snaps and GIFs with people in High Fidelity, Facebook, and Twitter.</p>' +
        '<br/>' +
        '<div style="text-align:center;font-weight:bold;">' +
            '<p>Press the big red button to take a snap!</p>' +
        '</div>';
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

function selectImageToShare(selectedID, isSelected) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var imageContainer = document.getElementById(selectedID),
        image = document.getElementById(selectedID + 'img'),
        shareBar = document.getElementById(selectedID + "shareBar"),
        shareButtonsDiv = document.getElementById(selectedID + "shareButtonsDiv"),
        showShareButtonsButton = document.getElementById(selectedID + "showShareButtonsButton"),
        itr,
        containers = document.getElementsByClassName("shareControls");

    if (isSelected) {
        showShareButtonsButton.onclick = function () { selectImageToShare(selectedID, false); };
        showShareButtonsButton.classList.remove("inactive");
        showShareButtonsButton.classList.add("active");

        image.onclick = function () { selectImageToShare(selectedID, false); };
        imageContainer.style.outline = "4px solid #00b4ef";
        imageContainer.style.outlineOffset = "-4px";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.5)";

        shareButtonsDiv.style.visibility = "visible";

        for (itr = 0; itr < containers.length; itr += 1) {
            var parentID = containers[itr].id.slice(0, 2);
            if (parentID !== selectedID) {
                selectImageToShare(parentID, false);
            }
        }
    } else {
        showShareButtonsButton.onclick = function () { selectImageToShare(selectedID, true); };
        showShareButtonsButton.classList.remove("active");
        showShareButtonsButton.classList.add("inactive");

        image.onclick = function () { selectImageToShare(selectedID, true); };
        imageContainer.style.outline = "none";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.0)";

        shareButtonsDiv.style.visibility = "hidden";
    }
}
function createShareBar(parentID, isGif, blastButtonDisabled, hifiButtonDisabled) {
    var shareBar = document.createElement("div"),
        shareButtonsDivID = parentID + "shareButtonsDiv",
        showShareButtonsButtonDivID = parentID + "showShareButtonsButtonDiv",
        showShareButtonsButtonID = parentID + "showShareButtonsButton",
        showShareButtonsLabelID = parentID + "showShareButtonsLabel",
        blastToConnectionsButtonID = parentID + "blastToConnectionsButton",
        shareWithEveryoneButtonID = parentID + "shareWithEveryoneButton",
        facebookButtonID = parentID + "facebookButton",
        twitterButtonID = parentID + "twitterButton";

    shareBar.id = parentID + "shareBar";
    shareBar.className = "shareControls";
    shareBar.innerHTML = '<div class="shareButtons" id="' + shareButtonsDivID + '" style="visibility:hidden">' +
            '<input type="button"' + (blastButtonDisabled ? ' disabled' : '') + ' class="blastToConnections blueButton" id="' +
                blastToConnectionsButtonID + '" value="BLAST TO MY CONNECTIONS" onclick="blastToConnections(' + parentID + ', ' + isGif + ')" />' +
            '<input type="button"' + (hifiButtonDisabled ? ' disabled' : '') + ' class="shareWithEveryone" id="' +
                shareWithEveryoneButtonID + '" onclick="shareWithEveryone(' + parentID + ', ' + isGif + ')" />' +
            '<a class="facebookButton" id="' + facebookButtonID + '" onclick="shareButtonClicked(\'facebook\', ' + parentID + ')"></a>' +
            '<a class="twitterButton" id="' + twitterButtonID + '" onclick="shareButtonClicked(\'twitter\', ' + parentID + ')"></a>' +
        '</div>' +
        '<div class="showShareButtonsButtonDiv" id="' + showShareButtonsButtonDivID + '">' +
            '<label id="' + showShareButtonsLabelID + '" for="' + showShareButtonsButtonID + '">SHARE</label>' +
            '<input type="button" class="showShareButton inactive" id="' + showShareButtonsButtonID + '" onclick="selectImageToShare(' + parentID + ', true)" />' +
            '<div class="showShareButtonDots">' +
                '&#xe019;' +
            '</div>' +
        '</div>';

    // Add onclick handler to parent DIV's img to toggle share buttons
    document.getElementById(parentID + 'img').onclick = function () { selectImageToShare(parentID, true); };

    return shareBar;
}
function appendShareBar(divID, isGif, blastButtonDisabled, hifiButtonDisabled) {
    document.getElementById(divID).appendChild(createShareBar(divID, isGif, blastButtonDisabled, hifiButtonDisabled));
    if (divID === "p0") {
        selectImageToShare(divID, true);
    }
}
function shareForUrl(selectedID) {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "shareSnapshotForUrl",
        data: paths[parseInt(selectedID.substring(1), 10)]
    }));
}
function addImage(image_data, isGifLoading, canShare, isShowingPreviousImages, blastButtonDisabled, hifiButtonDisabled) {
    if (!image_data.localPath) {
        return;
    }
    var id = "p" + (idCounter++),
        imageContainer = document.createElement("DIV"),
        img = document.createElement("IMG"),
        isGif = img.src.split('.').pop().toLowerCase() === "gif";
    imageContainer.id = id;
    imageContainer.style.width = "95%";
    imageContainer.style.height = "240px";
    imageContainer.style.margin = "5px auto";
    imageContainer.style.display = "flex";
    imageContainer.style.justifyContent = "center";
    imageContainer.style.alignItems = "center";
    imageContainer.style.position = "relative";
    img.id = id + "img";
    img.src = image_data.localPath;
    imageContainer.appendChild(img);
    document.getElementById("snapshot-images").appendChild(imageContainer);
    paths.push(image_data.localPath);
    if (isGif) {
        imageContainer.innerHTML += '<span class="gifLabel">GIF</span>';
    }
    if (!isGifLoading && !isShowingPreviousImages && canShare) {
        shareForUrl(id);
        appendShareBar(id, isGif, blastButtonDisabled, hifiButtonDisabled);
    }
    if (isShowingPreviousImages && image_data.story_id) {
        appendShareBar(id, isGif, blastButtonDisabled, hifiButtonDisabled);
        updateShareInfo(id, image_data.story_id);
    }
}
function showUploadingMessage(selectedID, destination) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `containerID` is passed as an HTML object to these functions; we just want the ID
    }

    var uploadingMessage = document.createElement("div");
    uploadingMessage.id = selectedID + "uploadingMessage";
    uploadingMessage.className = "uploadingMessage";
    uploadingMessage.setAttribute("data-destination", destination);

    var socialIcon = document.createElement("img");
    switch (destination) {
        case 'blast':
            socialIcon.src = "img/shareIcon.png";
            break;
        case 'hifi':
            socialIcon.src = "img/shareToFeed.png";
            break;
        case 'facebook':
            socialIcon.src = "img/fb_logo72.png";
            break;
        case 'twitter':
            socialIcon.src = "img/twitter_logo72.png";
            break;
    }
    uploadingMessage.appendChild(socialIcon);

    uploadingMessage.innerHTML += '<span>Preparing Snapshot for Sharing...</span>';

    document.getElementById(selectedID).appendChild(uploadingMessage);
}
function hideUploadingMessage(selectedID, storyID) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `containerID` is passed as an HTML object to these functions; we just want the ID
    }

    var uploadingMessage = document.getElementById(selectedID + "uploadingMessage");
    if (uploadingMessage) {
        uploadingMessage.remove();
        var destination = uploadingMessage.getAttribute("data-destination");

        switch (destination) {
            case 'blast':
                blastToConnections(selectedID, selectedID === "p1");
                break;
            case 'hifi':
                shareWithEveryone(selectedID, selectedID === "p1");
                break;
            case 'facebook':
                var facebookButton = document.getElementById(selectedID + "facebookButton");
                window.open(facebookButton.getAttribute("href"), "_blank");
                break;
            case 'twitter':
                var twitterButton = document.getElementById(selectedID + "twitterButton");
                window.open(twitterButton.getAttribute("href"), "_blank");
                break;
        }

        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "removeFromStoryIDsToMaybeDelete",
            story_id: storyID
        }));
    }
}
function updateShareInfo(containerID, storyID) {
    if (containerID.id) {
        containerID = containerID.id; // sometimes (?), `containerID` is passed as an HTML object to these functions; we just want the ID
    }
    var shareBar = document.getElementById(containerID + "shareBar"),
        parentDiv = document.getElementById(containerID),
        shareURL = "https://highfidelity.com/user_stories/" + storyID,
        facebookButton = document.getElementById(containerID + "facebookButton"),
        twitterButton = document.getElementById(containerID + "twitterButton");

    parentDiv.setAttribute('data-story-id', storyID);

    facebookButton.setAttribute("target", "_blank");
    facebookButton.setAttribute("href", 'https://www.facebook.com/dialog/feed?app_id=1585088821786423&link=' + shareURL);

    twitterButton.setAttribute("target", "_blank");
    twitterButton.setAttribute("href", 'https://twitter.com/intent/tweet?text=I%20just%20took%20a%20snapshot!&url=' + shareURL + '&via=highfidelity&hashtags=VR,HiFi');

    hideUploadingMessage(containerID, storyID);
}
function blastToConnections(selectedID, isGif) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }

    document.getElementById(selectedID + "blastToConnectionsButton").disabled = true;

    var storyID = document.getElementById(selectedID).getAttribute("data-story-id");

    if (storyID) {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "blastToConnections",
            story_id: storyID,
            isGif: isGif
        }));
    } else {
        showUploadingMessage(selectedID, 'blast');
    }
}
function shareWithEveryone(selectedID, isGif) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }

    document.getElementById(selectedID + "shareWithEveryoneButton").disabled = true;

    var storyID = document.getElementById(selectedID).getAttribute("data-story-id");

    if (storyID) {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "shareSnapshotWithEveryone",
            story_id: storyID,
            isGif: isGif
        }));
    } else {
        showUploadingMessage(selectedID, 'hifi');
    }
}
function shareButtonClicked(destination, selectedID) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var storyID = document.getElementById(selectedID).getAttribute("data-story-id");

    if (!storyID) {
        showUploadingMessage(selectedID, destination);
    }
}

function handleCaptureSetting(setting) {
    var stillAndGif = document.getElementById('stillAndGif'),
        stillOnly = document.getElementById('stillOnly');

    stillAndGif.checked = setting;
    stillOnly.checked = !setting;

    stillAndGif.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "captureStillAndGif"
        }));
    };

    stillOnly.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "captureStillOnly"
        }));
    };

}
window.onload = function () {
    // Uncomment the line below to test functionality in a browser.
    // See definition of "testInBrowser()" to modify tests.
    //testInBrowser(2);
    openEventBridge(function () {
        // Set up a handler for receiving the data, and tell the .js we are ready to receive it.
        EventBridge.scriptEventReceived.connect(function (message) {

            message = JSON.parse(message);

            if (message.type !== "snapshot") {
                return;
            }

            var messageOptions = message.options;

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
                    imageCount = message.image_data.length;
                    if (imageCount > 0) {
                        message.image_data.forEach(function (element, idx) {
                            addImage(element, true, message.canShare, true, message.image_data[idx].blastButtonDisabled, message.image_data[idx].hifiButtonDisabled);
                        });
                    } else {
                        showSnapshotInstructions();
                    }
                    break;
                case 'addImages':
                    // The last element of the message contents list contains a bunch of options,
                    // including whether or not we can share stuff
                    // The other elements of the list contain image paths.

                    if (messageOptions.containsGif) {
                        if (messageOptions.processingGif) {
                            imageCount = message.image_data.length + 1; // "+1" for the GIF that'll finish processing soon
                            message.image_data.push({ localPath: messageOptions.loadingGifPath });
                            message.image_data.forEach(function (element, idx) {
                                addImage(element, idx === 1, idx === 0 && messageOptions.canShare, false);
                            });
                        } else {
                            var gifPath = message.image_data[0].localPath,
                                p1img = document.getElementById('p1img');
                            p1img.src = gifPath;

                            paths[1] = gifPath;
                            if (messageOptions.canShare) {
                                shareForUrl("p1");
                                appendShareBar("p1", true, false, false);
                            }
                        }
                    } else {
                        imageCount = message.image_data.length;
                        message.image_data.forEach(function (element) {
                            addImage(element, false, messageOptions.canShare, false);
                        });
                    }
                    break;
                case 'captureSettings':
                    handleCaptureSetting(message.setting);
                    break;
                case 'snapshotUploadComplete':
                    var isGif = message.image_url.split('.').pop().toLowerCase() === "gif";
                    updateShareInfo(isGif ? "p1" : "p0", message.story_id);
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

function testInBrowser(test) {
    if (test === 0) {
        showSetupInstructions();
    } else if (test === 1) {
        imageCount = 1;
        //addImage({ localPath: 'http://lorempixel.com/553/255' });
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.gif' }, false, true, true, false, false);
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.jpg' }, false, true, true, false, false);
    } else if (test === 2) {
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.gif' }, false, true, true, false, false);
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.jpg' }, false, true, true, false, false);
        showUploadingMessage("p0", 'facebook');
        showUploadingMessage("p1", 'twitter');
    }
}
