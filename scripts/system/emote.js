"use strict";

//
//  emote.js
//  scripts/system/
//
//  Created by Brad Hefta-Gaub on 7 Jan 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Script, Tablet */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE


var EMOTE_ANIMATIONS = ['Cry', 'Surprised', 'Dance', 'Cheer', 'Wave', 'Fall', 'Point', 'Clap', 'Sit1', 'Sit2', 'Sit3', 'Love'];
var ANIMATIONS = Array();

var eventMappingName = "io.highfidelity.away"; // restoreAnimation on hand controller button events, too.
var eventMapping = Controller.newMapping(eventMappingName);

EMOTE_ANIMATIONS.forEach(function (name) {
    var animationURL = Script.resolvePath("assets/animations/" + name + ".fbx");
    var resource = AnimationCache.prefetch(animationURL);
    var animation = AnimationCache.getAnimation(animationURL);
    ANIMATIONS[name] = { url: animationURL, animation: animation, resource: resource};
});


var EMOTE_APP_BASE = "html/EmoteApp.html";
var EMOTE_APP_URL = Script.resolvePath(EMOTE_APP_BASE);
var EMOTE_LABEL = "EMOTE";
var EMOTE_APP_SORT_ORDER = 12;
var FPS = 60;
var MSEC_PER_SEC = 1000;
var FINISHED = 3; // see ScriptableResource::State

var onEmoteScreen = false;
var button;
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var activeTimer = false; // used to cancel active timer if a user plays an amimation while another animation is playing
var activeEmote = false; // to keep track of the currently playing emote

button = tablet.addButton({
    icon: "icons/tablet-icons/EmoteAppIcon.svg",
    text: EMOTE_LABEL,
    sortOrder: EMOTE_APP_SORT_ORDER
});

function onClicked() {
    if (onEmoteScreen) {
        tablet.gotoHomeScreen();
    } else {
        onEmoteScreen = true;
        tablet.gotoWebScreen(EMOTE_APP_URL);
    }
}

function onScreenChanged(type, url) {
    onEmoteScreen = type === "Web" && (url.indexOf(EMOTE_APP_BASE) == url.length - EMOTE_APP_BASE.length);
    button.editProperties({ isActive: onEmoteScreen });
}

// Handle the events we're receiving from the web UI
function onWebEventReceived(event) {

    // Converts the event to a JavasScript Object
    if (typeof event === "string") {
        event = JSON.parse(event);
    }

    if (event.type === "click") {
        
        var emoteName = event.data;
        
        if (activeTimer !== false) {
            Script.clearTimeout(activeTimer);
        }
        
        // if the activeEmote is different from the chosen emote, then play the new emote. Other wise, 
        // this is a second click on the same emote as the activeEmote, and we will just stop it.
        if (activeEmote !== emoteName) {
            activeEmote = emoteName;
            
            // Allow for a random sitting animation when a user selects sit
            var randSit = Math.floor(Math.random() * 3) + 1;
            if(emoteName == "Sit"){
                emoteName = event.data + randSit; // "Sit1, Sit2, Sit3"
            }
            
            var frameCount = ANIMATIONS[emoteName].animation.frames.length;
            
            // Three types of emotes (non-looping end, non-looping return, looping)
            if(emoteName.match(/^Sit.*$/) || emoteName == "Fall") { // non-looping end
            
                MyAvatar.overrideAnimation(ANIMATIONS[emoteName].url, FPS, false, 0, frameCount);
                
            } else if (emoteName == "Love" || emoteName == "Surprised" || emoteName == "Cry"  || emoteName == "Point"){ // non-looping return
            
                MyAvatar.overrideAnimation(ANIMATIONS[emoteName].url, FPS, false, 0, frameCount);
                var timeOut = MSEC_PER_SEC * frameCount / FPS;
                activeTimer = Script.setTimeout(function () {
                    MyAvatar.restoreAnimation();
                    activeTimer = false;
                    activeEmote = false;
                }, timeOut);
                
            } else { // looping
            
                MyAvatar.overrideAnimation(ANIMATIONS[emoteName].url, FPS, true, 0, frameCount);
                
            }
            
        } else {
            activeEmote = false;
            MyAvatar.restoreAnimation();
        }
        

        
    }
}

function restoreAnimation() {
        MyAvatar.restoreAnimation();
}

Controller.keyPressEvent.connect(restoreAnimation)
// Note peek() so as to not interfere with other mappings.
eventMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LeftSecondaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LT).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LB).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LS).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LeftGrip).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RT).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RB).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RS).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RightGrip).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.Back).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.Start).peek().to(restoreAnimation);
Controller.enableMapping(eventMappingName);

button.clicked.connect(onClicked);
tablet.screenChanged.connect(onScreenChanged);
tablet.webEventReceived.connect(onWebEventReceived);

Script.scriptEnding.connect(function () {
    if (onEmoteScreen) {
        tablet.gotoHomeScreen();
    }
    button.clicked.disconnect(onClicked);
    tablet.screenChanged.disconnect(onScreenChanged);
    if (tablet) {
        tablet.removeButton(button);
    }
    if (activeTimer !== false) {
        Script.clearTimeout(activeTimer);
        MyAvatar.restoreAnimation();
    }
});


}()); // END LOCAL_SCOPE
