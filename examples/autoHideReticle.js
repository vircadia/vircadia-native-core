//  autoHideReticle.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/23/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This script will auto-hide the reticle on inactivity... and then if it detects movement after being hidden
//  it will automatically move the reticle to the look at position
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MINIMUM_SEEK_DISTANCE = 0.01;
var NON_LINEAR_DIVISOR = 2;

var lastMouseMove = Date.now();
var HIDE_STATIC_MOUSE_AFTER = 5000; // 5 seconds
var seekToLookAt = false;

Controller.mouseMoveEvent.connect(function(mouseEvent) {
    lastMouseMove = Date.now();

    // if the reticle is hidden, show it...
    if (!Reticle.visible) {
        Reticle.visible = true;
        if (HMD.active) {
            seekToLookAt = true;
        }
    }
});



Script.update.connect(function(deltaTime) {

    // if we're currently seeking the lookAt move the mouse toward the lookat
    if (HMD.active && seekToLookAt) {
        var lookAt2D = HMD.getHUDLookAtPosition2D();
        var currentReticlePosition = Reticle.position;
        var distanceBetweenX = lookAt2D.x - Reticle.position.x;
        var distanceBetweenY = lookAt2D.y - Reticle.position.y;
        var moveX = distanceBetweenX / NON_LINEAR_DIVISOR;
        var moveY = distanceBetweenY / NON_LINEAR_DIVISOR;
        var newPosition = { x: Reticle.position.x + moveX, y: Reticle.position.y + moveY };
        var closeEnoughX = false;
        var closeEnoughY = false;
        if (moveX < MINIMUM_SEEK_DISTANCE) {
            newPosition.x = lookAt2D.x;
            closeEnoughX = true;
        }
        if (moveY < MINIMUM_SEEK_DISTANCE) {
            newPosition.y = lookAt2D.y;
            closeEnoughY = true;
        }
        if (closeEnoughX && closeEnoughY) {
            seekToLookAt = false;
        }
        Reticle.position = newPosition;
    }

    // if we haven't moved in a long period of time, hide the reticle
    if (Reticle.visible) {
        var now = Date.now();
        var timeSinceLastMouseMove = now - lastMouseMove;
        if (timeSinceLastMouseMove > HIDE_STATIC_MOUSE_AFTER) {
            Reticle.visible = false;
        }
    }
});
