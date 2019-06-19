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


var EMOTE_ANIMATIONS = 
    ['Crying', 'Surprised', 'Dancing', 'Cheering', 'Waving', 'Fall', 'Pointing', 'Clapping', 'Sit1', 'Sit2', 'Sit3', 'Love'];
var ANIMATIONS = Array();

var eventMappingName = "io.highfidelity.away"; // restoreAnimation on hand controller button events, too
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
var activeTimer = false; // Used to cancel active timer if a user plays an animation while another animation is playing
var activeEmote = false; // To keep track of the currently playing emote

button = tablet.addButton({
    icon: "icons/tablet-icons/emote-i.svg",
    activeIcon: "icons/tablet-icons/emote-a.svg",
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
    onEmoteScreen = type === "Web" && (url.indexOf(EMOTE_APP_BASE) === url.length - EMOTE_APP_BASE.length);
    button.editProperties({ isActive: onEmoteScreen });
}

// Handle the events we're receiving from the web UI
function onWebEventReceived(event) {

    // Converts the event to a JavasScript Object
    if (typeof event === "string") {
        event = JSON.parse(event);
    }

    if (event.type === "click") {
        
        // Allow for a random sitting animation when a user selects sit
        var randSit = Math.floor(Math.random() * 3) + 1;
        
        var emoteName = event.data;
        
        if (emoteName === "Sit"){
            emoteName = event.data + randSit; // Sit1, Sit2, Sit3
        }
        
        if (ANIMATIONS[emoteName].resource.state === FINISHED) {
            
            if (activeTimer !== false) {
                Script.clearTimeout(activeTimer);
            }
            
            // If the activeEmote is different from the chosen emote, then play the new emote
            // This is a second click on the same emote as the activeEmote, and we will just stop it
            if (activeEmote !== emoteName) {
                activeEmote = emoteName;
                
                            
                // Sit is the only animation currently that plays and then ends at the last frame
                if (emoteName.match(/^Sit.*$/)) {
                
                    // If user provides input during a sit, the avatar animation state should be restored
                    Controller.keyPressEvent.connect(restoreAnimation);
                    Controller.enableMapping(eventMappingName);
                    MyAvatar.overrideAnimation(ANIMATIONS[emoteName].url, FPS, false, 0, frameCount);
                
                } else {
                    
                    activeEmote = emoteName;
                    var frameCount = ANIMATIONS[emoteName].animation.frames.length;
                    MyAvatar.overrideAnimation(ANIMATIONS[emoteName].url, FPS, false, 0, frameCount);

                    var timeOut = MSEC_PER_SEC * frameCount / FPS;
                    activeTimer = Script.setTimeout(function () {
                        MyAvatar.restoreAnimation();
                        activeTimer = false;
                        activeEmote = false;
                    }, timeOut);
                    
                }
                
            } else {
                activeEmote = false;
                MyAvatar.restoreAnimation();
            }
        }
    }
}

// Restore the navigation animation states (idle, walk, run)
function restoreAnimation() {
    MyAvatar.restoreAnimation();
    
    // Make sure the input is disconnected after animations are restored so it doesn't affect any emotes other than sit
    Controller.keyPressEvent.disconnect(restoreAnimation);
    Controller.disableMapping(eventMappingName);
}
                    
// Note peek() so as to not interfere with other mappings.
eventMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LeftSecondaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LB).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LS).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RY).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RX).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LY).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LX).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.LeftGrip).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RB).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RS).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.RightGrip).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.Back).peek().to(restoreAnimation);
eventMapping.from(Controller.Standard.Start).peek().to(restoreAnimation);


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
