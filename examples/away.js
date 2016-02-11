"use strict";
/*jslint vars: true, plusplus: true*/
/*global HMD, AudioDevice, MyAvatar, Controller, Script, Overlays, print*/
//
//  away.js
//  examples
//
//  Created by Howard Stearns 11/3/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Goes into "paused" when the '.' key (and automatically when started in HMD), and normal when pressing any key.
// See MAIN CONTROL, below, for what "paused" actually does.
var OVERLAY_RATIO = 1920 / 1080;
var OVERLAY_DATA = {
    imageURL: "http://hifi-content.s3.amazonaws.com/alan/production/images/images/Overlay-Viz-blank.png",
    color: {red: 255, green: 255, blue: 255},
    alpha: 1
};
// For the script to receive an event when the user presses a hand-controller button, we need to map
// that button to the handler, which we do at the bottom of this script. However, doing so changes
// the mapping for the user, across all scripts. Therefore, we just enable the mapping when going
// "away", and disable it when going "active". However, this is occuring within a Controller event, so we
// have to wait until the Controller code has finished with all such events, so we delay
// it. Additionally, we may go "away" on startup, and we want to wait until any other such mapping
// have occured, because only the last remapping wins.
var CONTROLLER_REMAPPING_DELAY = 750; // ms

// ANIMATION
// We currently don't have play/stopAnimation integrated with the animation graph, but we can get the same effect
// using an animation graph with a state that we turn on and off through the animation var defined with that state.
var awayAnimationHandlerId, activeAnimationHandlerId, stopper;
function playAwayAnimation() {
    function animateAway() {
        return {isAway: true, isNotAway: false, isNotMoving: false, ikOverlayAlpha: 0.0};
    }
    if (stopper) {
        stopper = false;
        MyAvatar.removeAnimationStateHandler(activeAnimationHandlerId); // do it now, before making new assignment
    }
    awayAnimationHandlerId = MyAvatar.addAnimationStateHandler(animateAway, null);
}
function stopAwayAnimation() {
    MyAvatar.removeAnimationStateHandler(awayAnimationHandlerId);
    if (stopper) {
        print('WARNING: unexpected double stop');
        return;
    }
    // How do we know when to turn ikOverlayAlpha back on?
    // It cannot be as soon as we want to stop the away animation, because then things will look goofy as we come out of that animation.
    // (Imagine an away animation that sits or kneels, and then stands back up when coming out of it. If head is at the HMD, then it won't
    //  want to track the standing up animation.)
    // The anim graph will trigger awayOutroOnDone when awayOutro is finished.
    var backToNormal = false;
    stopper = true;
    function animateActive(state) {
        if (state.awayOutroOnDone) {
            backToNormal = true;
            stopper = false;
        } else if (state.ikOverlayAlpha) {
            // Once the right state gets reflected back to us, we don't need the hander any more.
            // But we are locked against handler changes during the execution of a handler, so remove asynchronously.
            Script.setTimeout(function () { MyAvatar.removeAnimationStateHandler(activeAnimationHandlerId); }, 0);
        }
        // It might be cool to "come back to life" by fading the ik overlay back in over a short time. But let's see how this goes.
        return {isAway: false, isNotAway: true, ikOverlayAlpha: backToNormal ? 1.0 : 0.0}; // IWBNI we had a way of deleting an anim var.
    }
    activeAnimationHandlerId = MyAvatar.addAnimationStateHandler(animateActive, ['ikOverlayAlpha', 'awayOutroOnDone']);
}

// OVERLAY
var overlay = Overlays.addOverlay("image", OVERLAY_DATA);
function showOverlay() {
    var properties = {visible: true},
        // Update for current screen size, keeping overlay proportions constant.
        screen = Controller.getViewportDimensions(),
        screenRatio = screen.x / screen.y;
    if (screenRatio < OVERLAY_RATIO) {
        properties.width = screen.x;
        properties.height = screen.x / OVERLAY_RATIO;
        properties.x = 0;
        properties.y = (screen.y - properties.height) / 2;
    } else {
        properties.height = screen.y;
        properties.width = screen.y * OVERLAY_RATIO;
        properties.y = 0;
        properties.x = (screen.x - properties.width) / 2;
    }
    Overlays.editOverlay(overlay, properties);
}
function hideOverlay() {
    Overlays.editOverlay(overlay, {visible: false});
}
hideOverlay();


// MAIN CONTROL
var wasMuted, isAway;
var eventMappingName = "io.highfidelity.away"; // goActive on hand controller button events, too.
var eventMapping = Controller.newMapping(eventMappingName);

function goAway() {
    if (isAway) {
        return;
    }
    Script.setTimeout(function () {
	if (isAway) { // i.e., unless something changed during the delay.
	    Controller.enableMapping(eventMappingName);
	}
    }, CONTROLLER_REMAPPING_DELAY);
    isAway = true;
    print('going "away"');
    wasMuted = AudioDevice.getMuted();
    if (!wasMuted) {
        AudioDevice.toggleMute();
    }
    MyAvatar.setEnableMeshVisible(false);  // just for our own display, without changing point of view
    playAwayAnimation(); // animation is still seen by others
    showOverlay();
}
function goActive() {
    if (!isAway) {
        return;
    }
    Script.setTimeout(function () {
	if (!isAway) { // i.e., unless something changed during the delay
	    Controller.disableMapping(eventMappingName); // Let hand-controller events through to other scripts.
	}
    }, CONTROLLER_REMAPPING_DELAY);
    isAway = false;
    print('going "active"');
    if (!wasMuted) {
        AudioDevice.toggleMute();
    }
    MyAvatar.setEnableMeshVisible(true); // IWBNI we respected Developer->Avatar->Draw Mesh setting.
    stopAwayAnimation();
    hideOverlay();
}

function maybeGoActive(event) {
    if (event.isAutoRepeat) {  // isAutoRepeat is true when held down (or when Windows feels like it)
        return;
    }
    if (!isAway && (event.text === '.')) {
        goAway();
    } else {
        goActive();
    }
}
var wasHmdActive = false;
function maybeGoAway() {
    if (HMD.active !== wasHmdActive) {
        wasHmdActive = !wasHmdActive;
        if (wasHmdActive) {
            goAway();
        }
    }
}

Script.update.connect(maybeGoAway);
// These two can be set now and left active as long as the script is running. They do not interfere with other scripts.
Controller.mousePressEvent.connect(goActive);
Controller.keyPressEvent.connect(maybeGoActive);
// Set up the mapping, but don't enable it until we're actually away.
eventMapping.from(Controller.Standard.LeftPrimaryThumb).to(goActive);
eventMapping.from(Controller.Standard.RightPrimaryThumb).to(goActive);
eventMapping.from(Controller.Standard.LeftSecondaryThumb).to(goActive);
eventMapping.from(Controller.Standard.RightSecondaryThumb).to(goActive);

Script.scriptEnding.connect(function () {
    Script.update.disconnect(maybeGoAway);
    goActive();
});
