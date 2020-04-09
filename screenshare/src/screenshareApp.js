'use strict';
//  screenshareApp.js
//
//  Created by Milad Nazeri, Rebecca Stankus, and Zach Fox 2019/11/13
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const { remote } = require('electron');

// Helpers
function handleError(error) {
    if (error) {
        console.error(error);
    }
}


// When an application is picked, make sure we clear out the previous pick, toggle the page, 
// and add the correct source
let currentScreensharePickID = "";
function screensharePicked(id) {
    currentScreensharePickID = id;
    document.getElementById("share_pick").innerHTML = "";
    togglePage();
    addSource(sourceMap[id], "share_pick");
}


// Once we have confirmed that we want to share, prepare the tokbox publishing initiating
// and toggle back to the selects page
function screenConfirmed(isConfirmed) {    
    document.getElementById("selects").innerHTML = "";
    if (isConfirmed === true){
        onAccessApproved(currentScreensharePickID);
    }
    togglePage();
}


// Hide/show the select page or the confirmation page
let currentPage = "mainPage";
function togglePage(){
    if (currentPage === "mainPage") {
        currentPage = "confirmationPage";
        document.getElementById("select_screen").style.display = "none";
        document.getElementById("subtitle").innerHTML = "Confirm that you'd like to share this content.";
        document.getElementById("confirmation_screen").style.display = "block";
    } else {
        showSources();
        currentPage = "mainPage";
        document.getElementById("select_screen").style.display = "block";
        document.getElementById("subtitle").innerHTML = "Please select the content you'd like to share.";
        document.getElementById("confirmation_screen").style.display = "none";
    }
}


// UI

// Render the html properly and append that to the correct parent
function addSource(source, type) {
    let renderedHTML = renderSourceHTML(source);
    if (type === "selects") {
        document.getElementById("selects").appendChild(renderedHTML);
    } else {
        document.getElementById("share_pick").appendChild(renderedHTML);
        document.getElementById("content_name").innerHTML = source.name;
    }
}


// Get the html created from the source.  Alter slightly depending on whether this source
// is on the selects screen, or the confirmation screen.  Mainly removing highlighting.
// If there is an app Icon, then add it.
function renderSourceHTML(source) {
    let type = currentPage === "confirmationPage" ? "share_pick" : "selects";
    let sourceBody = document.createElement('div')
    let thumbnail = source.thumbnail.toDataURL();
    sourceBody.classList.add("box")
    if (type === "share_pick") {
        sourceBody.style.marginLeft = "0px";
    }

    let image = "";
    if (source.appIcon) {
        image = `<img class="icon" src="${source.appIcon.toDataURL()}" />`;
    }
    sourceBody.innerHTML = `
        <div class="heading">
            ${image}
            <span class="screen_label">${source.name}</span>
        </div>
        <div class="${type === "share_pick" ? "image_no_hover" : "image"}" onclick="screensharePicked('${source.id}')">
            <img src="${thumbnail}" />
        </div>
    `
    return sourceBody;
}


// Separate out the screenshares and applications
// Make sure the screens are labeled in order
// Concact the two arrays back together and return
function sortSources() {
    let screenSources = [];
    let applicationSources = [];
    // Difference with Mac selects:
    // 1 screen = "Enitre Screen", more than one like PC "Screen 1, Screen 2..."
    screenshareSourceArray.forEach((source) => {
        if (source.name.match(/(entire )?screen( )?([0-9]?)/i)) {
            screenSources.push(source);
        } else {
            applicationSources.push(source)
        }
    });
    screenSources.sort((a, b) => {
        let aNumber = a.name.replace(/[^\d]/, "");
        let bNumber = b.name.replace(/[^\d]/, "");
        return aNumber - bNumber;
    });
    let finalSources = [...screenSources, ...applicationSources];
    return finalSources;
}


// Setup sorting the selection array, add individual sources, and update the sourceMap
function addSources() {
    screenshareSourceArray = sortSources();
    for (let i = 0; i < screenshareSourceArray.length; i++) {
        addSource(screenshareSourceArray[i], "selects");
        sourceMap[screenshareSourceArray[i].id] = screenshareSourceArray[i];
    }
}


// 1. Get the screens and window that are available from electron
// 2. Remove the screenshare app itself
// 3. Create a source map to help grab the correct source when picked
// 4. push all the sources for sorting to the source array
// 5. Add thse sources
const electron = require('electron');
const SCREENSHARE_TITLE = "Screen share";
const SCREENSHARE_TITLE_REGEX = new RegExp("^" + SCREENSHARE_TITLE + "$");
const IMAGE_WIDTH = 265;
const IMAGE_HEIGHT = 165;
let screenshareSourceArray = [];
let sourceMap = {};    
function showSources() {
    screenshareSourceArray = [];
    electron.desktopCapturer.getSources({ 
        types:['window', 'screen'],
        thumbnailSize: {
            width: IMAGE_WIDTH,
            height: IMAGE_HEIGHT
        },
        fetchWindowIcons: true
    }, (error, sources) => {
        if (error) {
            console.log("Error getting sources", error);
        }
        for (let source of sources) {
            if (source.name.match(SCREENSHARE_TITLE_REGEX)){
                continue;
            }
            sourceMap[source.id] = source;
            screenshareSourceArray.push(source);
        }
        addSources();
    });
}


// Stop the localstream and end the tokrok publishing
let localStream;
let desktopSharing;
function stopSharing() {
    desktopSharing = false;

    if (localStream) {
        localStream.getTracks()[0].stop();
        localStream = null;
    }

    document.getElementById('screenshare').style.display = "none";
    stopTokBoxPublisher();
}


// Callback to start publishing after we have setup the chromium stream
function gotStream(stream) {
    if (localStream) {
        stopSharing();
    }

    localStream = stream;
    startTokboxPublisher(localStream);

    stream.onended = () => {
        if (desktopSharing) {
            togglePage();
        }
    };
}


// After we grant access to electron, create a stream and using the callback
// start the tokbox publisher
function onAccessApproved(desktop_id) {
    if (!desktop_id) {
        console.log('Desktop Capture access rejected.');
        return;
    }



    document.getElementById('screenshare').style.visibility = "block";
    desktopSharing = true;
    navigator.webkitGetUserMedia({
        audio: false,
        video: {
            mandatory: {
                chromeMediaSource: 'desktop',
                chromeMediaSourceId: desktop_id,
                maxWidth: 1280,
                maxHeight: 720,
                maxFrameRate: 7
            }
        }
    }, gotStream, handleError);
    remote.getCurrentWindow().minimize();
}


// Tokbox

// Once we have the connection info, this will create the session which will allow
// us to publish a stream when we are ready
function initializeTokboxSession() {
    session = OT.initSession(projectAPIKey, sessionID);
    session.on('sessionDisconnected', (event) => {
        console.log('You were disconnected from the session.', event.reason);
    });

    // Connect to the session
    session.connect(token, (error) => {
        if (error) {
            handleError(error);
        }
    });
}


// Init the tokbox publisher with our newly created stream
var publisher; 
function startTokboxPublisher(stream) {
    publisher = document.createElement("div");
    var publisherOptions = {
        audioFallbackEnabled: false,
        audioSource: null,
        fitMode: 'contain',
        frameRate: 7,
        height: 720,
        insertMode: 'append',
        publishAudio: false,
        videoSource: stream.getVideoTracks()[0],
        width: 1280
    };

    publisher = OT.initPublisher(publisher, publisherOptions, function(error){
        if (error) {
            console.log("ERROR: " + error);
        } else {
            session.publish(publisher, function(error) {
                if (error) {
                    console.log("ERROR FROM Session.publish: " + error);
                    return;
                }
            })
        }
    });
}


// Kills the streaming being sent to tokbox
function stopTokBoxPublisher() {
    publisher.destroy();
}


// When the app is ready, we get this info from the command line arguments.
const ipcRenderer = electron.ipcRenderer;
let projectAPIKey;
let sessionID;
let token;
let session;
ipcRenderer.on('connectionInfo', function(event, message) {
    const connectionInfo = JSON.parse(message);
    projectAPIKey = connectionInfo.projectAPIKey;
    sessionID = connectionInfo.sessionID;
    token = connectionInfo.token;

    initializeTokboxSession();
});


// Show the initial sources after the dom has loaded
// Sources come from electron.desktopCapturer
document.addEventListener("DOMContentLoaded", () => {
    showSources();
});
