//
//  progress.js
//  examples
//
//  Created by David Rowe on 29 Jan 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script displays a progress download indicator when downloads are in progress.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var rawProgress = 100, // % raw value.
        displayProgress = 100, // % smoothed value to display.
        DISPLAY_PROGRESS_MINOR_MAXIMUM = 8, // % displayed progress bar goes up to while 0% raw progress.
        DISPLAY_PROGRESS_MINOR_INCREMENT = 0.1, // % amount to increment display value each update when 0% raw progress.
        DISPLAY_PROGRESS_MAJOR_INCREMENT = 5, // % maximum amount to increment display value when >0% raw progress.
        alpha = 0.0,
        alphaDelta = 0.0, // > 0 if fading in; < 0 if fading out.
        ALPHA_DELTA_IN = 0.15,
        ALPHA_DELTA_OUT = -0.02,
        fadeTimer = null,
        FADE_INTERVAL = 30, // ms between changes in alpha.
        fadeWaitTimer = null,
        FADE_OUT_WAIT = 1000, // Wait before starting to fade out after progress 100%.
        visible = false,
        BAR_WIDTH = 480, // Dimension of SVG in pixels of visible portion (half) of the bar.
        BAR_HEIGHT = 30,
        BAR_URL = Script.resolvePath("assets/images/progress-bar.svg"),
        BACKGROUND_WIDTH = 540,
        BACKGROUND_HEIGHT = 90,
        BACKGROUND_URL = Script.resolvePath("assets/images/progress-bar-background.svg"),
        isOnHMD = false,
        windowWidth = 0,
        windowHeight = 0,
        background2D = {},
        bar2D = {},
        SCALE_2D = 0.35, // Scale the SVGs for 2D display.
        //SCALE_2D = 1.0, // Scale the SVGs for 2D display.
        background3D = {},
        bar3D = {},
        PROGRESS_3D_DIRECTION = 0.0, // Degrees from avatar orientation.
        PROGRESS_3D_DISTANCE = 0.602, // Horizontal distance from avatar position.
        PROGRESS_3D_ELEVATION = -0.8, // Height of top middle of top notification relative to avatar eyes.
        PROGRESS_3D_YAW = 0.0, // Degrees relative to notifications direction.
        PROGRESS_3D_PITCH = -60.0, // Degrees from vertical.
        SCALE_3D = 0.0011, // Scale the bar SVG for 3D display.
        BACKGROUND_3D_SIZE = {
            x: 0.76,
            y: 0.08
        }, // Match up with the 3D background with those of notifications.js notices.
        BACKGROUND_3D_COLOR = {
            red: 2,
            green: 2,
            blue: 2
        },
        BACKGROUND_3D_ALPHA = 0.7;

    function fade() {

        alpha = alpha + alphaDelta;

        if (alpha < 0) {
            alpha = 0;
        }

        if (alpha > 1) {
            alpha = 1;
        }

        if (alpha === 0 || alpha === 1) { // Finished fading in or out
            alphaDelta = 0;
            Script.clearInterval(fadeTimer);
        }

        if (alpha === 0) { // Finished fading out
            visible = false;
        }

        if (isOnHMD) {
            Overlays.editOverlay(background3D.overlay, {
                backgroundAlpha: alpha * BACKGROUND_3D_ALPHA,
                visible: visible
            });
        } else {
            Overlays.editOverlay(background2D.overlay, {
                alpha: alpha,
                visible: visible
            });
        }
        Overlays.editOverlay(isOnHMD ? bar3D.overlay : bar2D.overlay, {
            alpha: alpha,
            visible: visible
        });
    }
    function resetProgress() {
        wasActive = true;
        bestRawProgress = 0;
        rawProgress = 0;
        cooldown = 1000;
        displayProgress = 0;
    }

    Window.domainChanged.connect(resetProgress);

    var maxSeen = 0;
    var bestRawProgress = 0;
    var wasActive = false;
    var cooldown = 1000;
    function onDownloadInfoChanged(info) {
        var i;

        print("PROGRESS: DOwnload info changed ", info.downloading.length, info.pending, maxSeen);
        // Update raw progress value
        if (info.downloading.length + info.pending === 0) {
            wasActive = false;
            rawProgress = 100;
            bestRawProgress = 100;
        } else {
            var count = info.downloading.length + info.pending;
            if (!wasActive) {
                resetProgress();
                maxSeen = count;
            }
            if (count > maxSeen) {
                maxSeen = count;
            }
            if (cooldown < 0) {
                print("PROGRESS: OUT OF COOLDOWN");
                //for (i = 0; i < info.downloading.length; i += 1) {
                    //rawProgress += info.downloading[i];
                //}
                //rawProgress = rawProgress / (info.downloading.length + info.pending);
                rawProgress = ((maxSeen - count) / maxSeen) * 100;
                //rawProgress += 1;
                //if (rawProgress > 90) {
                    //rawProgress = 20
                //}

                if (rawProgress > bestRawProgress) {
                    bestRawProgress = rawProgress;
                }
            }
        }
        print("PROGRESS:", rawProgress, bestRawProgress, maxSeen);
    }

    function createOverlays() {
        if (isOnHMD) {

            background3D.overlay = Overlays.addOverlay("rectangle3d", {
                size: BACKGROUND_3D_SIZE,
                color: BACKGROUND_3D_COLOR,
                alpha: BACKGROUND_3D_ALPHA,
                solid: true,
                isFacingAvatar: false,
                visible: false,
                ignoreRayIntersection: true
            });
            bar3D.overlay = Overlays.addOverlay("image3d", {
                url: BAR_URL,
                subImage: {
                    x: 0,
                    y: 0,
                    width: 480,
                    height: BAR_HEIGHT
                },
                scale: SCALE_3D * BAR_WIDTH,
                isFacingAvatar: false,
                visible: false,
                alpha: 0.0,
                ignoreRayIntersection: true
            });

        } else {

            background2D.overlay = Overlays.addOverlay("image", {
                imageURL: BACKGROUND_URL,
                width: background2D.width,
                height: background2D.height,
                visible: false,
                alpha: 0.0
            });
            bar2D.overlay = Overlays.addOverlay("image", {
                imageURL: BAR_URL,
                subImage: {
                    x: 0,
                    y: 0,
                    width: BAR_WIDTH,
                    height: BAR_HEIGHT
                },
                width: bar2D.width,
                height: bar2D.height,
                visible: false,
                alpha: 0.0
            });
        }
    }

    function deleteOverlays() {
        Overlays.deleteOverlay(isOnHMD ? background3D.overlay : background2D.overlay);
        Overlays.deleteOverlay(isOnHMD ? bar3D.overlay : bar2D.overlay);
    }

    var b = 0;
    function update() {
        cooldown -= 16;
        /*
        maxSeen = 100;
        b++;
        rawProgress = b;
        if (rawProgress == 100) {
            b = 0
        }
        bestRawProgress = rawProgress;
        print(rawProgress, bestRawProgress);
        */

        //print(rawProgress, bestRawProgress, maxSeen);
        var viewport,
            eyePosition,
            avatarOrientation;

        hmdActive = HMD.active;
        //if (isOnHMD !== HMD.active) {
            //deleteOverlays();
            //isOnHMD = !isOnHMD;
            //createOverlays();
        //}

        /*
        // Calculate progress value to display
        if (rawProgress === 0 && displayProgress <= DISPLAY_PROGRESS_MINOR_MAXIMUM) {
            displayProgress = Math.min(displayProgress + DISPLAY_PROGRESS_MINOR_INCREMENT, DISPLAY_PROGRESS_MINOR_MAXIMUM);
        } else if (rawProgress < displayProgress) {
            displayProgress = rawProgress;
        } else if (rawProgress > displayProgress) {
            displayProgress = Math.min(rawProgress, displayProgress + DISPLAY_PROGRESS_MAJOR_INCREMENT);
        } // else (rawProgress === displayProgress); do nothing.
        //displayProgress = bestRawProgress;
        //*/
        if (displayProgress < rawProgress) {
            var diff = rawProgress - displayProgress;
            if (diff < 0.1) {
                displayProgress = rawProgress;
            } else {
                displayProgress += diff * 0.1;
            }
        }
        print('PROGRESS:', displayProgress);


        // Update state
        if (!visible) { // Not visible because no recent downloads
            if (displayProgress < 100) { // Have started downloading so fade in
                visible = true;
                alphaDelta = ALPHA_DELTA_IN;
                fadeTimer = Script.setInterval(fade, FADE_INTERVAL);
            }
        } else if (alphaDelta !== 0.0) { // Fading in or out
            if (alphaDelta > 0) {
                if (rawProgress === 100) { // Was downloading but now have finished so fade out
                    alphaDelta = ALPHA_DELTA_OUT;
                }
            } else {
                if (displayProgress < 100) { // Was finished downloading but have resumed so fade in
                    alphaDelta = ALPHA_DELTA_IN;
                }
            }
        } else { // Fully visible because downloading or recently so
            if (fadeWaitTimer === null) {
                if (rawProgress === 100) { // Was downloading but have finished so fade out soon
                    fadeWaitTimer = Script.setTimeout(function() {
                        alphaDelta = ALPHA_DELTA_OUT;
                        fadeTimer = Script.setInterval(fade, FADE_INTERVAL);
                        fadeWaitTimer = null;
                    }, FADE_OUT_WAIT);
                }
            } else {
                if (displayProgress < 100) { // Was finished and waiting to fade out but have resumed so don't fade out
                    Script.clearInterval(fadeWaitTimer);
                    fadeWaitTimer = null;
                }
            }
        }

        if (visible) {

            // Update progress bar
            Overlays.editOverlay(isOnHMD ? bar3D.overlay : bar2D.overlay, {
                visible: visible,
                subImage: {
                    x: BAR_WIDTH * (1 - displayProgress / 100),
                    y: 0,
                    width: BAR_WIDTH,
                    height: BAR_HEIGHT
                },
            });

            // Update position
            if (isOnHMD) {
                // Update 3D overlays to maintain positions relative to avatar
                eyePosition = MyAvatar.getDefaultEyePosition();
                avatarOrientation = MyAvatar.orientation;

                Overlays.editOverlay(background3D.overlay, {
                    position: Vec3.sum(eyePosition, Vec3.multiplyQbyV(avatarOrientation, background3D.offset)),
                    rotation: Quat.multiply(avatarOrientation, background3D.orientation)
                });
                Overlays.editOverlay(bar3D.overlay, {
                    position: Vec3.sum(eyePosition, Vec3.multiplyQbyV(avatarOrientation, bar3D.offset)),
                    rotation: Quat.multiply(avatarOrientation, bar3D.orientation)
                });

            } else {
                // Update 2D overlays to maintain positions at bottom middle of window
                viewport = Controller.getViewportDimensions();

                if (viewport.x !== windowWidth || viewport.y !== windowHeight) {
                    windowWidth = viewport.x;
                    windowHeight = viewport.y;

                    var yOffset = hmdActive ? -300 : 0;

                    Overlays.editOverlay(background2D.overlay, {
                        x: windowWidth / 2 - background2D.width / 2,
                        y: windowHeight - background2D.height - bar2D.height + yOffset
                    });

                    Overlays.editOverlay(bar2D.overlay, {
                        x: windowWidth / 2 - bar2D.width / 2,
                        y: windowHeight - background2D.height - bar2D.height + (background2D.height - bar2D.height) / 2 + yOffset
                    });
                }
            }
        }
    }

    function setProgressBar(progress) {
    }

    function setUp() {
        background2D.width = SCALE_2D * BACKGROUND_WIDTH;
        background2D.height = SCALE_2D * BACKGROUND_HEIGHT;
        bar2D.width = SCALE_2D * BAR_WIDTH;
        bar2D.height = SCALE_2D * BAR_HEIGHT;

        background3D.offset = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, PROGRESS_3D_DIRECTION, 0), {
            x: 0,
            y: 0,
            z: -PROGRESS_3D_DISTANCE
        });
        background3D.offset.y += PROGRESS_3D_ELEVATION;
        background3D.orientation = Quat.fromPitchYawRollDegrees(PROGRESS_3D_PITCH, PROGRESS_3D_DIRECTION + PROGRESS_3D_YAW, 0);
        bar3D.offset = Vec3.sum(background3D.offset, {
            x: 0,
            y: 0,
            z: 0.001
        }); // Just in front of background
        bar3D.orientation = background3D.orientation;

        createOverlays();
    }

    function tearDown() {
        deleteOverlays();
    }

    setUp();
    GlobalServices.downloadInfoChanged.connect(onDownloadInfoChanged);
    GlobalServices.updateDownloadInfo();
    //Script.update.connect(update);
    Script.setInterval(update, 16);
    Script.scriptEnding.connect(tearDown);
}());
