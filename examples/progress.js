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

(function () {

    var progress = 100,                             // %
        alpha = 0.0,
        alphaDelta = 0.0,                           // > 0 if fading in; < 0 if fading out/
        ALPHA_DELTA_IN = 0.15,
        ALPHA_DELTA_OUT = -0.02,
        fadeTimer = null,
        FADE_INTERVAL = 30,                         // ms between changes in alpha.
        fadeWaitTimer = null,
        FADE_OUT_WAIT = 1000,                       // Wait before starting to fade out after progress 100%.
        visible = false,
        BAR_WIDTH = 320,                            // Nominal dimension of SVG in pixels of visible portion (half) of the bar.
        BAR_HEIGHT = 20,
        BAR_URL = "http://ctrlaltstudio.com/hifi/progress-bar.svg",
        BACKGROUND_WIDTH = 360,
        BACKGROUND_HEIGHT = 60,
        BACKGROUND_URL = "http://ctrlaltstudio.com/hifi/progress-bar-background.svg",
        isOnHMD = false,
        windowWidth = 0,
        windowHeight = 0,
        background2D = {},
        bar2D = {},
        SCALE_2D = 0.55,                            // Scale the SVGs for 2D display.
        background3D = {},
        bar3D = {},
        ENABLE_VR_MODE_MENU_ITEM = "Enable VR Mode",
        PROGRESS_3D_DIRECTION = 0.0,                // Degrees from avatar orientation.
        PROGRESS_3D_DISTANCE = 0.602,               // Horizontal distance from avatar position.
        PROGRESS_3D_ELEVATION = -0.8,               // Height of top middle of top notification relative to avatar eyes.
        PROGRESS_3D_YAW = 0.0,                      // Degrees relative to notifications direction.
        PROGRESS_3D_PITCH = -60.0,                  // Degrees from vertical.
        SCALE_3D = 0.0017,                          // Scale the bar SVG for 3D display.
        BACKGROUND_3D_SIZE = { x: 0.76, y: 0.08 },  // Match up with the 3D background with those of notifications.js notices.
        BACKGROUND_3D_COLOR = { red: 2, green: 2, blue: 2 },
        BACKGROUND_3D_ALPHA = 0.7;

    function fade() {

        alpha = alpha + alphaDelta;

        if (alpha < 0) {
            alpha = 0;
        }

        if (alpha > 1) {
            alpha = 1;
        }

        if (alpha === 0 || alpha === 1) {  // Finished fading in or out
            alphaDelta = 0;
            Script.clearInterval(fadeTimer);
        }

        if (alpha === 0) {  // Finished fading out
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

    function onDownloadInfoChanged(info) {
        var i;

        // Calculate progress
        if (info.downloading.length + info.pending === 0) {
            progress = 100;
        } else {
            progress = 0;
            for (i = 0; i < info.downloading.length; i += 1) {
                progress += info.downloading[i];
            }
            progress = progress / (info.downloading.length + info.pending);
        }

        // Update state
        if (!visible) {  // Not visible because no recent downloads
            if (progress < 100) {  // Have started downloading so fade in
                visible = true;
                alphaDelta = ALPHA_DELTA_IN;
                fadeTimer = Script.setInterval(fade, FADE_INTERVAL);
            }
        } else if (alphaDelta !== 0.0) {  // Fading in or out
            if (alphaDelta > 0) {
                if (progress === 100) {  // Was donloading but now have finished so fade out
                    alphaDelta = ALPHA_DELTA_OUT;
                }
            } else {
                if (progress < 100) {  // Was finished downloading but have resumed so fade in
                    alphaDelta = ALPHA_DELTA_IN;
                }
            }
        } else {  // Fully visible because downloading or recently so
            if (fadeWaitTimer === null) {
                if (progress === 100) {  // Was downloading but have finished so fade out soon
                    fadeWaitTimer = Script.setTimeout(function () {
                        alphaDelta = ALPHA_DELTA_OUT;
                        fadeTimer = Script.setInterval(fade, FADE_INTERVAL);
                        fadeWaitTimer = null;
                    }, FADE_OUT_WAIT);
                }
            } else {
                if (progress < 100) {  // Was finished and waiting to fade out but have resumed downloading so don't fade out
                    Script.clearInterval(fadeWaitTimer);
                    fadeWaitTimer = null;
                }
            }
        }

        // Update progress bar
        if (visible) {
            Overlays.editOverlay(isOnHMD ? bar3D.overlay : bar2D.overlay, {
                subImage: { x: BAR_WIDTH * (1 - progress / 100), y: 0, width: BAR_WIDTH, height: BAR_HEIGHT }
            });
        }
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
            bar3D.overlay = Overlays.addOverlay("billboard", {
                url: BAR_URL,
                subImage: { x: BAR_WIDTH, y: 0, width: BAR_WIDTH, height: BAR_HEIGHT },
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
                subImage: { x: BAR_WIDTH, y: 0, width: BAR_WIDTH, height: BAR_HEIGHT },
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

    function update() {
        var viewport,
            eyePosition,
            avatarOrientation;

        if (isOnHMD !== Menu.isOptionChecked(ENABLE_VR_MODE_MENU_ITEM)) {
            deleteOverlays();
            isOnHMD = !isOnHMD;
            createOverlays();
        }

        if (visible) {
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

                    Overlays.editOverlay(background2D.overlay, {
                        x: windowWidth / 2 - background2D.width / 2,
                        y: windowHeight - background2D.height - bar2D.height
                    });

                    Overlays.editOverlay(bar2D.overlay, {
                        x: windowWidth / 2 - bar2D.width / 2,
                        y: windowHeight - background2D.height - bar2D.height + (background2D.height - bar2D.height) / 2
                    });
                }
            }
        }
    }

    function setUp() {
        background2D.width = SCALE_2D * BACKGROUND_WIDTH;
        background2D.height = SCALE_2D * BACKGROUND_HEIGHT;
        bar2D.width = SCALE_2D * BAR_WIDTH;
        bar2D.height = SCALE_2D * BAR_HEIGHT;

        background3D.offset = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, PROGRESS_3D_DIRECTION, 0),
            { x: 0, y: 0, z: -PROGRESS_3D_DISTANCE });
        background3D.offset.y += PROGRESS_3D_ELEVATION;
        background3D.orientation = Quat.fromPitchYawRollDegrees(PROGRESS_3D_PITCH, PROGRESS_3D_DIRECTION + PROGRESS_3D_YAW, 0);
        bar3D.offset = Vec3.sum(background3D.offset, { x: 0, y: 0, z: 0.001 });  // Just in front of background
        bar3D.orientation = background3D.orientation;

        createOverlays();
    }

    function tearDown() {
        deleteOverlays();
    }

    setUp();
    GlobalServices.downloadInfoChanged.connect(onDownloadInfoChanged);
    GlobalServices.updateDownloadInfo();
    Script.update.connect(update);
    Script.scriptEnding.connect(tearDown);
}());
