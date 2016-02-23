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
var wasMouseCaptured = false;
function maybeGoAway() {
    if (HMD.active !== wasHmdActive) {
        wasHmdActive = !wasHmdActive;
        if (wasHmdActive) {
            goAway();
        }
    }

    // If the mouse has gone from captured, to non-captured state,
    // then it likely means the person is still in the HMD, but has
    // tabbed away from the application (meaning they don't have mouse 
    // control) and they likely want to go into an away state
    if (Reticle.mouseCaptured !== wasMouseCaptured) {
        wasMouseCaptured = !wasMouseCaptured;
        if (!wasMouseCaptured) {
            goAway();
        }
    }
}

Script.update.connect(maybeGoAway);
Controller.mousePressEvent.connect(goActive);
Controller.keyPressEvent.connect(maybeGoActive);
// Note peek() so as to not interfere with other mappings.
eventMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(goActive); 
eventMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(goActive);
eventMapping.from(Controller.Standard.LeftSecondaryThumb).peek().to(goActive);
eventMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(goActive);
Controller.enableMapping(eventMappingName);

Script.scriptEnding.connect(function () {
    Script.update.disconnect(maybeGoAway);
    goActive();
    Controller.disableMapping(eventMappingName);
    Controller.mousePressEvent.disconnect(goActive);
    Controller.keyPressEvent.disconnect(maybeGoActive);
});
