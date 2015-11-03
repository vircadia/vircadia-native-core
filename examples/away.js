"use strict";
/*jslint vars: true, plusplus: true*/
/*global HMD, AudioDevice, MyAvatar, Controller, Script, Overlays, print*/

// Goes into "paused" when the '.' key (and automatically when started in HMD), and normal when pressing any key.
// See MAIN CONTROL, below, for what "paused" actually does.

var OVERLAY_DATA = {
    text: "Paused:\npress any key to continue",
    font: {size: 75},
    color: {red: 200, green: 255, blue: 255},
    alpha: 0.9
};

// ANIMATION
// We currently don't have play/stopAnimation integrated with the animation graph, but we can get the same effect
// using an animation graph with a state that we turn on and off through the animation var defined with that state.
var awayAnimationHandlerId, activeAnimationHandlerId;
function playAwayAnimation() {
    function animateAway() {
        return {isAway: true, isNotAway: false, isNotMoving: false};
    }
    awayAnimationHandlerId = MyAvatar.addAnimationStateHandler(animateAway, null);
}
function stopAwayAnimation() {
    MyAvatar.removeAnimationStateHandler(awayAnimationHandlerId);
    function animateActive(state) {
        if (!state.isAway) { // Once the right state gets reflected back to us, we don't need the hander any more.
            // But we are locked against handler changes during the execution of a handler, so remove asynchronously.
            Script.setTimeout(function () { MyAvatar.removeAnimationStateHandler(activeAnimationHandlerId); }, 0);
        }
        return {isAway: false, isNotAway: true}; // IWBNI we had a way of deleting an anim var.
    }
    activeAnimationHandlerId = MyAvatar.addAnimationStateHandler(animateActive, ['isAway']);
}

// OVERLAY
var overlay = Overlays.addOverlay("text", OVERLAY_DATA);
function showOverlay() {
    var screen = Controller.getViewportDimensions();
    Overlays.editOverlay(overlay, {visible: true, x: screen.x / 4, y: screen.y / 4});
}
function hideOverlay() {
    Overlays.editOverlay(overlay, {visible: false});
}
hideOverlay();


// MAIN CONTROL
var wasMuted, isAway;
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
function goActive(event) {
    if (!isAway) {
        if (event.text === '.') {
            goAway();
        }
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
Controller.keyPressEvent.connect(goActive);
if (HMD.active) {
    goAway();
}
