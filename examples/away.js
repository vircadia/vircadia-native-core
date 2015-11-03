"use strict";
/*jslint vars: true, plusplus: true*/
/*global HMD, AudioDevice, MyAvatar, Controller, Script, Overlays, print*/

// Goes into "paused" when the '.' key (and automatically when started in HMD), and normal when pressing any key.
// See MAIN CONTROL, below, for what "paused" actually does.

var IK_WINDOW_AFTER_GOING_ACTIVE = 3000; // milliseconds
var OVERLAY_DATA = {
    text: "Paused:\npress any key to continue",
    font: {size: 75},
    color: {red: 200, green: 255, blue: 255},
    alpha: 0.9
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
	Script.clearTimeout(stopper);
	MyAvatar.removeAnimationStateHandler(activeAnimationHandlerId); // do it now, before making new assignment
    }
    awayAnimationHandlerId = MyAvatar.addAnimationStateHandler(animateAway, null);
}
function stopAwayAnimation() {
    MyAvatar.removeAnimationStateHandler(awayAnimationHandlerId);
    if (stopper) { print('WARNING: unexpected double stop'); return; }
    // How do we know when to turn ikOverlayAlpha back on?
    // It cannot be as soon as we want to stop the away animation, because then things will look goofy as we come out of that animation.
    // (Imagine an away animation that sits or kneels, and then stands back up when coming out of it. If head is at the HMD, then it won't
    //  want to track the standing up animation.)
    // Our standard anim graph flips 'awayOutroOnDone' for one frame, but it's a trigger (not an animVar) and other folks might use different graphs.
    // So... Just give us a fixed amount of time to be done with animation, before we turn ik back on.
    var backToNormal = false;
    stopper = Script.setTimeout(function () {
	backToNormal = true;
	stopper = false;
    }, IK_WINDOW_AFTER_GOING_ACTIVE);
    function animateActive(state) {
	if (state.ikOverlayAlpha) {
	    // Once the right state gets reflected back to us, we don't need the hander any more.
	    // But we are locked against handler changes during the execution of a handler, so remove asynchronously.
	    Script.setTimeout(function () { MyAvatar.removeAnimationStateHandler(activeAnimationHandlerId); }, 0);
	}
	// It might be cool to "come back to life" by fading the ik overlay back in over a short time. But let's see how this goes.
        return {isAway: false, isNotAway: true, ikOverlayAlpha: backToNormal ? 1.0 : 0.0}; // IWBNI we had a way of deleting an anim var.
    }
    activeAnimationHandlerId = MyAvatar.addAnimationStateHandler(animateActive, ['isAway', 'isNotAway', 'isNotMoving', 'ikOverlayAlpha']);
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
function goAway(event) {
    if (isAway || event.isAutoRepeat) { // isAutoRepeat is true when held down (or when Windows feels like it)
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
    if (!isAway || event.isAutoRepeat) {
        if (event.text === '.') {
            goAway(event);
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
Script.scriptEnding.connect(goActive);
if (HMD.active) {
    goAway({}); // give a dummy event object
}
