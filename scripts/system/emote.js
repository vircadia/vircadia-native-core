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


var EMOTE_ANIMATIONS = ['Crying', 'Surprised', 'Dancing', 'Cheering', 'Waving', 'Fall', 'Pointing', 'Clapping'];
var ANIMATIONS = Array();


EMOTE_ANIMATIONS.forEach(function (name) {
    var animURL = Script.resolvePath("assets/animations/" + name + ".fbx");
    print(animURL);
    var resource = AnimationCache.prefetch(animURL);
    var animation = AnimationCache.getAnimation(animURL);
    ANIMATIONS[name] = { url: animURL, animation: animation, resource: resource};
});


var EMOTE_APP_BASE = "html/EmoteApp.html";
var EMOTE_APP_URL = Script.resolvePath(EMOTE_APP_BASE);


var onEmoteScreen = false;
var button;
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

var emoteLable = "EMOTE";

button = tablet.addButton({
    //icon: "icons/tablet-icons/emote.svg",
    text: emoteLable,
    sortOrder: 11
});

var enabled = false;
function onClicked() {
    if (onEmoteScreen) {
        tablet.gotoHomeScreen();
    } else {
        onEmoteScreen = true;
        tablet.gotoWebScreen(EMOTE_APP_URL);
    }
}

function onScreenChanged(type, url) {
    print("type:", type, "url:", url);
    onEmoteScreen = type === "Web" && (url.indexOf(EMOTE_APP_BASE) == url.length - EMOTE_APP_BASE.length);
    button.editProperties({ isActive: onEmoteScreen });
}

// Handle the events we're receiving from the web UI
function onWebEventReceived(event) {
   print("emote.js received a web event: " + event);

    // Converts the event to a JavasScript Object
    if (typeof event === "string") {
        event = JSON.parse(event);
    }

    if (event.type === "click") {
        var emoteName = event.data;
        print("emote.js received a click event for " + emoteName);

        print("ANIMATIONS[emoteName].resource.url:" + ANIMATIONS[emoteName].resource.url);


        // ANIMATIONS[emoteName].animation.isLoaded
        if (ANIMATIONS[emoteName].resource.state == 3) {
            var frameCount = ANIMATIONS[emoteName].animation.frames.length;
            print("the animation for " + emoteName + " is loaded, it has " + frameCount + " frames");
            var frameCount = ANIMATIONS[emoteName].animation.frames.length;

            var FPS = 30;
            var MSEC_PER_SEC = 1000;
            MyAvatar.overrideAnimation(ANIMATIONS[emoteName].url, FPS, false, 0, frameCount);

            var timeOut = MSEC_PER_SEC * frameCount / FPS;
            Script.setTimeout(function () {
                MyAvatar.restoreAnimation();
            }, timeOut);


        } else {
            print("the animation for " + emoteName + " is in state:" + ANIMATIONS[emoteName].resource.state);
        }
    }
}

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
});


}()); // END LOCAL_SCOPE
