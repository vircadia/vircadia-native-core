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
var blastShareText = "Blast to my Connections",
    blastAlreadySharedText = "Already Blasted to Connections",
    hifiShareText = "Share to Snaps Feed",
    hifiAlreadySharedText = "Already Shared to Snaps Feed",
    facebookShareText = "Share to Facebook",
    twitterShareText = "Share to Twitter",
    shareButtonLabelTextActive = "SHARE&#58;",
    shareButtonLabelTextInactive = "SHARE";

function fileExtensionMatches(filePath, extension) {
    return filePath.split('.').pop().toLowerCase() === extension;
}

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
    document.getElementById("snap-button").disabled = false;
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
function login() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "login"
    }));
}
function clearImages() {
    var snapshotImagesDiv = document.getElementById("snapshot-images");
    snapshotImagesDiv.classList.remove("snapshotInstructions");
    while (snapshotImagesDiv.hasChildNodes()) {
        snapshotImagesDiv.removeChild(snapshotImagesDiv.lastChild);
    }
    paths = [];
    imageCount = 0;
    idCounter = 0;
}

function selectImageWithHelpText(selectedID, isSelected) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var imageContainer = document.getElementById(selectedID),
        image = document.getElementById(selectedID + 'img'),
        shareBar = document.getElementById(selectedID + "shareBar"),
        helpTextDiv = document.getElementById(selectedID + "helpTextDiv"),
        showShareButtonsButtonDiv = document.getElementById(selectedID + "showShareButtonsButtonDiv"),
        itr,
        containers = document.getElementsByClassName("shareControls");

    if (isSelected) {
        showShareButtonsButtonDiv.onclick = function () { selectImageWithHelpText(selectedID, false); };
        showShareButtonsButtonDiv.classList.remove("inactive");
        showShareButtonsButtonDiv.classList.add("active");

        image.onclick = function () { selectImageWithHelpText(selectedID, false); };
        imageContainer.style.outline = "4px solid #00b4ef";
        imageContainer.style.outlineOffset = "-4px";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.45)";
        shareBar.style.pointerEvents = "initial";

        helpTextDiv.style.visibility = "visible";

        for (itr = 0; itr < containers.length; itr += 1) {
            var parentID = containers[itr].id.slice(0, 2);
            if (parentID !== selectedID) {
                selectImageWithHelpText(parentID, false);
            }
        }
    } else {
        showShareButtonsButtonDiv.onclick = function () { selectImageWithHelpText(selectedID, true); };
        showShareButtonsButtonDiv.classList.remove("active");
        showShareButtonsButtonDiv.classList.add("inactive");

        image.onclick = function () { selectImageWithHelpText(selectedID, true); };
        imageContainer.style.outline = "none";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.0)";
        shareBar.style.pointerEvents = "none";

        helpTextDiv.style.visibility = "hidden";
    }
}
function selectImageToShare(selectedID, isSelected) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var imageContainer = document.getElementById(selectedID),
        image = document.getElementById(selectedID + 'img'),
        shareBar = document.getElementById(selectedID + "shareBar"),
        showShareButtonsDots = document.getElementById(selectedID + "showShareButtonsDots"),
        showShareButtonsLabel = document.getElementById(selectedID + "showShareButtonsLabel"),
        shareButtonsDiv = document.getElementById(selectedID + "shareButtonsDiv"),
        shareBarHelp = document.getElementById(selectedID + "shareBarHelp"),
        showShareButtonsButtonDiv = document.getElementById(selectedID + "showShareButtonsButtonDiv"),
        itr,
        containers = document.getElementsByClassName("shareControls");

    if (isSelected) {
        showShareButtonsButtonDiv.onclick = function () { selectImageToShare(selectedID, false); };
        showShareButtonsButtonDiv.classList.remove("inactive");
        showShareButtonsButtonDiv.classList.add("active");

        image.onclick = function () { selectImageToShare(selectedID, false); };
        imageContainer.style.outline = "4px solid #00b4ef";
        imageContainer.style.outlineOffset = "-4px";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.45)";
        shareBar.style.pointerEvents = "initial";

        showShareButtonsDots.style.visibility = "hidden";
        showShareButtonsLabel.innerHTML = shareButtonLabelTextActive;

        shareButtonsDiv.style.visibility = "visible";
        shareBarHelp.style.visibility = "visible";

        for (itr = 0; itr < containers.length; itr += 1) {
            var parentID = containers[itr].id.slice(0, 2);
            if (parentID !== selectedID) {
                selectImageToShare(parentID, false);
            }
        }
    } else {
        showShareButtonsButtonDiv.onclick = function () { selectImageToShare(selectedID, true); };
        showShareButtonsButtonDiv.classList.remove("active");
        showShareButtonsButtonDiv.classList.add("inactive");

        image.onclick = function () { selectImageToShare(selectedID, true); };
        imageContainer.style.outline = "none";

        shareBar.style.backgroundColor = "rgba(0, 0, 0, 0.0)";
        shareBar.style.pointerEvents = "none";

        showShareButtonsDots.style.visibility = "visible";
        showShareButtonsLabel.innerHTML = shareButtonLabelTextInactive;

        shareButtonsDiv.style.visibility = "hidden";
        shareBarHelp.style.visibility = "hidden";
    }
}
function createShareBar(parentID, isLoggedIn, canShare, isGif, blastButtonDisabled, hifiButtonDisabled, canBlast) {
    var shareBar = document.createElement("div"),
        shareBarHelpID = parentID + "shareBarHelp",
        shareButtonsDivID = parentID + "shareButtonsDiv",
        showShareButtonsButtonDivID = parentID + "showShareButtonsButtonDiv",
        showShareButtonsDotsID = parentID + "showShareButtonsDots",
        showShareButtonsLabelID = parentID + "showShareButtonsLabel",
        blastToConnectionsButtonID = parentID + "blastToConnectionsButton",
        shareWithEveryoneButtonID = parentID + "shareWithEveryoneButton",
        facebookButtonID = parentID + "facebookButton",
        twitterButtonID = parentID + "twitterButton",
        shareBarInnerHTML = '';

    shareBar.id = parentID + "shareBar";
    shareBar.className = "shareControls";

    if (isLoggedIn) {
        if (canShare) {
            shareBarInnerHTML = '<div class="shareControlsHelp" id="' + shareBarHelpID + '" style="visibility:hidden;' + ((canBlast && blastButtonDisabled || !canBlast && hifiButtonDisabled) ? "background-color:#000;opacity:0.5;" : "") + '"></div>' +
                '<div class="showShareButtonsButtonDiv inactive" id="' + showShareButtonsButtonDivID + '" onclick="selectImageToShare(' + parentID + ', true)">' +
                    '<label id="' + showShareButtonsLabelID + '">' + shareButtonLabelTextInactive + '</label>' +
                    '<span id="' + showShareButtonsDotsID + '" class="showShareButtonDots">' +
                        '&#xe019;' +
                    '</div>' +
                '</div>' +
                '<div class="shareButtons" id="' + shareButtonsDivID + '" style="visibility:hidden">';
            if (canBlast) {
                shareBarInnerHTML += '<div class="shareButton blastToConnections' + (blastButtonDisabled ? ' disabled' : '') + '" id="' + blastToConnectionsButtonID + '" onmouseover="shareButtonHovered(\'blast\', ' + parentID + ', true)" onclick="' + (blastButtonDisabled ? '' : 'blastToConnections(' + parentID + ', ' + isGif + ')') + '"><img src="img/blast_icon.svg"></div>';
            }
            shareBarInnerHTML += '<div class="shareButton shareWithEveryone' + (hifiButtonDisabled ? ' disabled' : '') + '" id="' + shareWithEveryoneButtonID + '" onmouseover="shareButtonHovered(\'hifi\', ' + parentID + ', true)" onclick="' + (hifiButtonDisabled ? '' : 'shareWithEveryone(' + parentID + ', ' + isGif + ')') + '"><img src="img/hifi_icon.svg" style="width:35px;height:35px;margin:2px 0 0 2px;"></div>' +
                    '<a class="shareButton facebookButton" id="' + facebookButtonID + '" onmouseover="shareButtonHovered(\'facebook\', ' + parentID + ', true)" onclick="shareButtonClicked(\'facebook\', ' + parentID + ')"><img src="img/fb_icon.svg"></a>' +
                    '<a class="shareButton twitterButton" id="' + twitterButtonID + '" onmouseover="shareButtonHovered(\'twitter\', ' + parentID + ', true)" onclick="shareButtonClicked(\'twitter\', ' + parentID + ')"><img src="img/twitter_icon.svg"></a>' +
                '</div>';

            // Add onclick handler to parent DIV's img to toggle share buttons
            document.getElementById(parentID + 'img').onclick = function () { selectImageToShare(parentID, true); };
        } else {
            shareBarInnerHTML = '<div class="showShareButtonsButtonDiv inactive" id="' + showShareButtonsButtonDivID + '" onclick="selectImageToShare(' + parentID + ', true)">' +
                    '<label id="' + showShareButtonsLabelID + '">' + shareButtonLabelTextInactive + '</label>' +
                    '<span class="showShareButtonDots">' +
                        '&#xe019;' +
                    '</div>' +
                '</div>' +
                '<div class="helpTextDiv" id="' + parentID + 'helpTextDiv' + '" style="visibility:hidden;text-align:left;">' +
                    'This snap was taken in an unshareable domain.' +
                '</div>';
            // Add onclick handler to parent DIV's img to toggle share buttons
            document.getElementById(parentID + 'img').onclick = function () { selectImageWithHelpText(parentID, true); };
        }
    } else {
        shareBarInnerHTML = '<div class="showShareButtonsButtonDiv inactive" id="' + showShareButtonsButtonDivID + '" onclick="selectImageToShare(' + parentID + ', true)">' +
                '<label id="' + showShareButtonsLabelID + '">' + shareButtonLabelTextInactive + '</label>' +
                '<span class="showShareButtonDots">' +
                    '&#xe019;' +
                '</div>' +
            '</div>' +
            '<div class="helpTextDiv" id="' + parentID + 'helpTextDiv' + '" style="visibility:hidden;text-align:right;">' +
                'Please log in to share snaps' + '<input class="grayButton" style="margin-left:20px;width:95px;height:30px;" type="button" value="LOG IN" onclick="login()" />' +
            '</div>';
        // Add onclick handler to parent DIV's img to toggle share buttons
        document.getElementById(parentID + 'img').onclick = function () { selectImageWithHelpText(parentID, true); };
    }

    shareBar.innerHTML = shareBarInnerHTML;

    return shareBar;
}
function appendShareBar(divID, isLoggedIn, canShare, isGif, blastButtonDisabled, hifiButtonDisabled, canBlast) {
    if (divID.id) {
        divID = divID.id; // sometimes (?), `containerID` is passed as an HTML object to these functions; we just want the ID
    }
    document.getElementById(divID).appendChild(createShareBar(divID, isLoggedIn, canShare, isGif, blastButtonDisabled, hifiButtonDisabled, canBlast));
    if (divID === "p0") {
        if (isLoggedIn) {
            if (canShare) {
                selectImageToShare(divID, true);
            } else {
                selectImageWithHelpText(divID, true);
            }
        } else {
            selectImageWithHelpText(divID, true);
        }
    }
    if (isLoggedIn && canShare) {
        if (canBlast) {
            shareButtonHovered('blast', divID, false);
        } else {
            shareButtonHovered('hifi', divID, false);
        }
    }
}
function shareForUrl(selectedID) {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "shareSnapshotForUrl",
        data: paths[parseInt(selectedID.substring(1), 10)]
    }));
}
function addImage(image_data, isLoggedIn, canShare, isGifLoading, isShowingPreviousImages, blastButtonDisabled, hifiButtonDisabled, canBlast) {
    if (!image_data.localPath) {
        return;
    }
    var imageContainer = document.createElement("DIV"),
        img = document.createElement("IMG"),
        isGif = fileExtensionMatches(image_data.localPath, "gif"),
        id = "p" + (isGif ? "1" : "0");
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
    img.onload = function () {
        if (isGif) {
            imageContainer.innerHTML += '<span class="gifLabel">GIF</span>';
        }
        if (!isGifLoading) {
            appendShareBar(id, isLoggedIn, canShare, isGif, blastButtonDisabled, hifiButtonDisabled, canBlast);
        }
        if ((!isShowingPreviousImages && ((isGif && !isGifLoading) || !isGif)) || (isShowingPreviousImages && !image_data.story_id)) {
            shareForUrl(id);
        }
        if (isShowingPreviousImages && isLoggedIn && image_data.story_id) {
            updateShareInfo(id, image_data.story_id);
        }
        if (isShowingPreviousImages) {
            requestPrintButtonUpdate();  
        } 
    };
    img.onerror = function () {
        img.onload = null;
        img.src = image_data.errorPath;
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "alertSnapshotLoadFailed"
        }));
    };
}
function showConfirmationMessage(selectedID, destination) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `containerID` is passed as an HTML object to these functions; we just want the ID
    }

    var opacity = 2.0,
        fadeRate = 0.05,
        timeBetweenFadesMS = 50,
        confirmationMessageContainer = document.createElement("div"),
        confirmationMessage = document.createElement("div");
    confirmationMessageContainer.className = "confirmationMessageContainer";

    confirmationMessage.className = "confirmationMessage";

    var socialIcon = document.createElement("img");
    switch (destination) {
        case 'blast':
            socialIcon.src = "img/blast_icon.svg";
            confirmationMessage.appendChild(socialIcon);
            confirmationMessage.innerHTML += '<span>Blast Sent!</span>';
            confirmationMessage.style.backgroundColor = "#EA4C5F";
            break;
        case 'hifi':
            socialIcon.src = "img/hifi_icon.svg";
            confirmationMessage.appendChild(socialIcon);
            confirmationMessage.innerHTML += '<span>Snap Shared!</span>';
            confirmationMessage.style.backgroundColor = "#1FC6A6";
            break;
    }

    confirmationMessageContainer.appendChild(confirmationMessage);
    document.getElementById(selectedID).appendChild(confirmationMessageContainer);

    setInterval(function () {
        if (opacity <= fadeRate) {
            confirmationMessageContainer.remove();
        }
        opacity -= fadeRate;
        confirmationMessageContainer.style.opacity = opacity;
    }, timeBetweenFadesMS);
}
function showUploadingMessage(selectedID, destination) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `containerID` is passed as an HTML object to these functions; we just want the ID
    }

    var shareBarHelp = document.getElementById(selectedID + "shareBarHelp");

    shareBarHelp.innerHTML = '<img style="display:inline;width:25px;height:25px;" src="./img/loader.gif"></img><span style="position:relative;margin-left:5px;bottom:7px;">Preparing to Share</span>';
    shareBarHelp.classList.add("uploading");
    shareBarHelp.setAttribute("data-destination", destination);
}
function hideUploadingMessageAndMaybeShare(selectedID, storyID) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `containerID` is passed as an HTML object to these functions; we just want the ID
    }

    var shareBarHelp = document.getElementById(selectedID + "shareBarHelp"),
        shareBarHelpDestination = shareBarHelp.getAttribute("data-destination");

    shareBarHelp.classList.remove("uploading");
    if (shareBarHelpDestination) {
        switch (shareBarHelpDestination) {
            case 'blast':
                blastToConnections(selectedID, selectedID === "p1");
                shareBarHelp.innerHTML = blastAlreadySharedText;
                break;
            case 'hifi':
                shareWithEveryone(selectedID, selectedID === "p1");
                shareBarHelp.innerHTML = hifiAlreadySharedText;
                break;
            case 'facebook':
                var facebookButton = document.getElementById(selectedID + "facebookButton");
                window.open(facebookButton.getAttribute("href"), "_blank");
                shareBarHelp.innerHTML = facebookShareText;
                // This emitWebEvent() call isn't necessary in the "hifi" and "blast" cases
                // because the "removeFromStoryIDsToMaybeDelete()" call happens
                // in snapshot.js when sharing with that method.
                EventBridge.emitWebEvent(JSON.stringify({
                    type: "snapshot",
                    action: "removeFromStoryIDsToMaybeDelete",
                    story_id: storyID
                }));
                break;
            case 'twitter':
                var twitterButton = document.getElementById(selectedID + "twitterButton");
                window.open(twitterButton.getAttribute("href"), "_blank");
                shareBarHelp.innerHTML = twitterShareText;
                EventBridge.emitWebEvent(JSON.stringify({
                    type: "snapshot",
                    action: "removeFromStoryIDsToMaybeDelete",
                    story_id: storyID
                }));
                break;
        }

        shareBarHelp.setAttribute("data-destination", "");
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
    twitterButton.setAttribute("href", 'https://twitter.com/intent/tweet?text=I%20just%20took%20a%20snapshot!&url=' + shareURL + '&via=highfidelityinc&hashtags=VR,HiFi');

    hideUploadingMessageAndMaybeShare(containerID, storyID);
}
function blastToConnections(selectedID, isGif) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }

    var blastToConnectionsButton = document.getElementById(selectedID + "blastToConnectionsButton"),
        shareBar = document.getElementById(selectedID + "shareBar"),
        shareBarHelp = document.getElementById(selectedID + "shareBarHelp");
    blastToConnectionsButton.onclick = function () { };

    var storyID = document.getElementById(selectedID).getAttribute("data-story-id");

    if (storyID) {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "blastToConnections",
            story_id: storyID,
            isGif: isGif
        }));
        showConfirmationMessage(selectedID, 'blast');
        blastToConnectionsButton.classList.add("disabled");
        blastToConnectionsButton.style.backgroundColor = "#000000";
        blastToConnectionsButton.style.opacity = "0.5";
        shareBarHelp.style.backgroundColor = "#000000";
        shareBarHelp.style.opacity = "0.5";
    } else {
        showUploadingMessage(selectedID, 'blast');
    }
}
function shareWithEveryone(selectedID, isGif) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }

    var shareWithEveryoneButton = document.getElementById(selectedID + "shareWithEveryoneButton"),
        shareBar = document.getElementById(selectedID + "shareBar"),
        shareBarHelp = document.getElementById(selectedID + "shareBarHelp");
    shareWithEveryoneButton.onclick = function () { };

    var storyID = document.getElementById(selectedID).getAttribute("data-story-id");

    if (storyID) {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "shareSnapshotWithEveryone",
            story_id: storyID,
            isGif: isGif
        }));
        showConfirmationMessage(selectedID, 'hifi');
        shareWithEveryoneButton.classList.add("disabled");
        shareWithEveryoneButton.style.backgroundColor = "#000000";
        shareWithEveryoneButton.style.opacity = "0.5";
        shareBarHelp.style.backgroundColor = "#000000";
        shareBarHelp.style.opacity = "0.5";
    } else {
        showUploadingMessage(selectedID, 'hifi');
    }
}
function shareButtonHovered(destination, selectedID, shouldAlsoModifyOther) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var shareBarHelp = document.getElementById(selectedID + "shareBarHelp"),
        shareButtonsDiv = document.getElementById(selectedID + "shareButtonsDiv").childNodes,
        itr;

    if (!shareBarHelp.classList.contains("uploading")) {
        for (itr = 0; itr < shareButtonsDiv.length; itr += 1) {
            shareButtonsDiv[itr].style.backgroundColor = "rgba(0, 0, 0, 0)";
        }
        shareBarHelp.style.opacity = "1.0";
        switch (destination) {
            case 'blast':
                var blastToConnectionsButton = document.getElementById(selectedID + "blastToConnectionsButton");
                if (!blastToConnectionsButton.classList.contains("disabled")) {
                    shareBarHelp.style.backgroundColor = "#EA4C5F";
                    shareBarHelp.style.opacity = "1.0";
                    blastToConnectionsButton.style.backgroundColor = "#EA4C5F";
                    blastToConnectionsButton.style.opacity = "1.0";
                    shareBarHelp.innerHTML = blastShareText;
                } else {
                    shareBarHelp.style.backgroundColor = "#000000";
                    shareBarHelp.style.opacity = "0.5";
                    blastToConnectionsButton.style.backgroundColor = "#000000";
                    blastToConnectionsButton.style.opacity = "0.5";
                    shareBarHelp.innerHTML = blastAlreadySharedText;
                }
                break;
            case 'hifi':
                var shareWithEveryoneButton = document.getElementById(selectedID + "shareWithEveryoneButton");
                if (!shareWithEveryoneButton.classList.contains("disabled")) {
                    shareBarHelp.style.backgroundColor = "#1FC6A6";
                    shareBarHelp.style.opacity = "1.0";
                    shareWithEveryoneButton.style.backgroundColor = "#1FC6A6";
                    shareWithEveryoneButton.style.opacity = "1.0";
                    shareBarHelp.innerHTML = hifiShareText;
                } else {
                    shareBarHelp.style.backgroundColor = "#000000";
                    shareBarHelp.style.opacity = "0.5";
                    shareWithEveryoneButton.style.backgroundColor = "#000000";
                    shareWithEveryoneButton.style.opacity = "0.5";
                    shareBarHelp.innerHTML = hifiAlreadySharedText;
                }
                break;
            case 'facebook':
                shareBarHelp.style.backgroundColor = "#3C58A0";
                shareBarHelp.innerHTML = facebookShareText;
                document.getElementById(selectedID + "facebookButton").style.backgroundColor = "#3C58A0";
                break;
            case 'twitter':
                shareBarHelp.style.backgroundColor = "#00B4EE";
                shareBarHelp.innerHTML = twitterShareText;
                document.getElementById(selectedID + "twitterButton").style.backgroundColor = "#00B4EE";
                break;
        }
    }

    if (shouldAlsoModifyOther && imageCount > 1) {
        if (selectedID === "p0" && !document.getElementById("p1").classList.contains("processingGif")) {
            shareButtonHovered(destination, "p1", false);
        } else if (selectedID === "p1") {
            shareButtonHovered(destination, "p0", false);
        }
    }
}
function shareButtonClicked(destination, selectedID) {
    if (selectedID.id) {
        selectedID = selectedID.id; // sometimes (?), `selectedID` is passed as an HTML object to these functions; we just want the ID
    }
    var storyID = document.getElementById(selectedID).getAttribute("data-story-id");

    if (!storyID) {
        showUploadingMessage(selectedID, destination);
    } else {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "removeFromStoryIDsToMaybeDelete",
            story_id: storyID
        }));
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
    //testInBrowser(4);
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
                            addImage(element, messageOptions.isLoggedIn, message.canShare, false, true, message.image_data[idx].blastButtonDisabled, message.image_data[idx].hifiButtonDisabled, messageOptions.canBlast);
                        });
                    } else {
                        showSnapshotInstructions();
                    }
                    break;
                case 'addImages':
                    // The last element of the message contents list contains a bunch of options,
                    // including whether or not we can share stuff
                    // The other elements of the list contain image paths.
                    if (messageOptions.containsGif === true) {
                        if (messageOptions.processingGif === true) {
                            imageCount = message.image_data.length + 1; // "+1" for the GIF that'll finish processing soon
                            message.image_data.push({ localPath: messageOptions.loadingGifPath });
                            message.image_data.forEach(function (element, idx) {
                                addImage(element, messageOptions.isLoggedIn, idx === 0 && messageOptions.canShare, idx === 1, false, false, false, true);
                            });
                            document.getElementById("p1").classList.add("processingGif");
                        } else {
                            var gifPath = message.image_data[0].localPath,
                                p1img = document.getElementById('p1img');
                            p1img.src = gifPath;

                            paths[1] = gifPath;
                            shareForUrl("p1");
                            appendShareBar("p1", messageOptions.isLoggedIn, messageOptions.canShare, true, false, false, messageOptions.canBlast);
                            document.getElementById("p1").classList.remove("processingGif");
                            document.getElementById("snap-button").disabled = false;
                        }
                    } else {
                        imageCount = message.image_data.length;
                        message.image_data.forEach(function (element) {
                            addImage(element, messageOptions.isLoggedIn, messageOptions.canShare, false, false, false, false, true);
                        });
                    }
                    break;
                case 'captureSettings':
                    handleCaptureSetting(message.setting);
                    break;              
                case 'setPrintButtonEnabled':
                    setPrintButtonEnabled();
                    break;
                case 'setPrintButtonLoading':
                    setPrintButtonLoading();
                    break;
                case 'setPrintButtonDisabled':
                    setPrintButtonDisabled();
                    break;
                case 'snapshotUploadComplete':
                    var isGif = fileExtensionMatches(message.image_url, "gif");
                    updateShareInfo(isGif ? "p1" : "p0", message.story_id);
                    if (isPrintProcessing()) {                       
                        setPrintButtonEnabled();
                    }
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
    if (document.getElementById('stillAndGif').checked === true) {
        document.getElementById("snap-button").disabled = true;
    }
}

function isPrintDisabled() { 
    var printElement = document.getElementById('print-icon');
    
    return printElement.classList.contains("print-icon") &&
           printElement.classList.contains("print-icon-default") &&
           document.getElementById('print-button').disabled;
}
function isPrintProcessing() { 
    var printElement = document.getElementById('print-icon');  
    
    return printElement.classList.contains("print-icon") &&
           printElement.classList.contains("print-icon-loading") &&
           document.getElementById('print-button').disabled;
}
function isPrintEnabled() {
    var printElement = document.getElementById('print-icon');    
    
    return printElement.classList.contains("print-icon") &&
           printElement.classList.contains("print-icon-default") &&
           !document.getElementById('print-button').disabled;
}

function setPrintButtonLoading() {
    document.getElementById('print-icon').className = "print-icon print-icon-loading";
    document.getElementById('print-button').disabled = true;
}
function setPrintButtonDisabled() {
    document.getElementById('print-icon').className = "print-icon print-icon-default";
    document.getElementById('print-button').disabled = true;
}
function setPrintButtonEnabled() { 
    document.getElementById('print-button').disabled = false;
    document.getElementById('print-icon').className = "print-icon print-icon-default";
}

function requestPrintButtonUpdate() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "snapshot",
        action: "requestPrintButtonUpdate"
    }));
}

function printToPolaroid() {   
    if (isPrintEnabled()) {        
        EventBridge.emitWebEvent(JSON.stringify({
            type: "snapshot",
            action: "printToPolaroid"
        }));       
    } else {
        setPrintButtonLoading();
    }
}

function testInBrowser(test) {
    if (test === 0) {
        showSetupInstructions();
    } else if (test === 1) {
        imageCount = 2;
        //addImage({ localPath: 'http://lorempixel.com/553/255' });
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.jpg', story_id: 1338 }, true, true, false, true, false, false, true);
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.gif', story_id: 1337 }, true, true, false, true, false, false, true);
    } else if (test === 2) {
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.jpg', story_id: 1338 }, true, true, false, true, false, false, true);
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.gif', story_id: 1337 }, true, true, false, true, false, false, true);
        showConfirmationMessage("p0", 'blast');
        showConfirmationMessage("p1", 'hifi');
    } else if (test === 3) {
        imageCount = 2;
        //addImage({ localPath: 'http://lorempixel.com/553/255' });
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.jpg', story_id: 1338 }, true, true, false, true, false, false, true);
        addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.gif', story_id: 1337 }, true, true, false, true, false, false, true);
        showUploadingMessage("p0", 'hifi');
    } else if (test === 4) {
    imageCount = 2;
    //addImage({ localPath: 'http://lorempixel.com/553/255' });
    addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.jpg', story_id: 1338 }, false, true, false, true, false, false, true);
    addImage({ localPath: 'D:/Dropbox/Screenshots/High Fidelity Snapshots/hifi-snap-by-zfox-on-2017-05-01_13-28-58.gif', story_id: 1337 }, false, true, false, true, false, false, true);
}
}
