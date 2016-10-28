"use strict";

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

(function () { // BEGIN LOCAL_SCOPE

    function debug() {
        //print.apply(null, arguments);
    }

    var rawProgress = 100, // % raw value.
        displayProgress = 100, // % smoothed value to display.
        alpha = 0.0,
        alphaDelta = 0.0, // > 0 if fading in; < 0 if fading out.
        ALPHA_DELTA_IN = 0.15,
        ALPHA_DELTA_OUT = -0.02,
        fadeTimer = null,
        FADE_INTERVAL = 30, // ms between changes in alpha.
        fadeWaitTimer = null,
        FADE_OUT_WAIT = 1000, // Wait before starting to fade out after progress 100%.
        visible = false,

        BAR_DESKTOP_WIDTH = 2240, // Width of SVG image in pixels. Sized for 1920 x 1080 display with 6 visible repeats.
        BAR_DESKTOP_REPEAT = 320,  // Length of repeat in bar = 2240 / 7.
        BAR_DESKTOP_HEIGHT = 3,  // Display height of SVG
        BAR_DESKTOP_URL = Script.resolvePath("assets/images/progress-bar-2k.svg"),

        BAR_HMD_WIDTH = 4430, // Width of SVG image in pixels. Sized for Rift with 6 visible repeats.
        BAR_HMD_REPEAT = 585,  // Length of repeat in bar = 4430 / 7.
        BAR_HMD_HEIGHT = 4,  // Display height of SVG
        BAR_HMD_URL = Script.resolvePath("assets/images/progress-bar-hmd.svg"),

        BAR_Y_OFFSET_2D = -10, // Offset of progress bar while in desktop mode
        BAR_Y_OFFSET_HMD = -300, // Offset of progress bar while in HMD

        TEXT_HEIGHT = 32,
        TEXT_WIDTH = 256,
        TEXT_URL = Script.resolvePath("assets/images/progress-bar-text.svg"),
        windowWidth = 0,
        windowHeight = 0,
        barDesktop = {},
        barHMD = {},
        textDesktop = {},  // Separate desktop and HMD overlays because can't change text size after overlay created.
        textHMD = {},
        SCALE_TEXT_DESKTOP = 0.6,
        SCALE_TEXT_HMD = 1.0,
        isHMD = false,

        // Max seen since downloads started. This is reset when all downloads have completed.
        maxSeen = 0,

        // Progress is defined as: (pending_downloads + active_downloads) / max_seen
        // We keep track of both the current progress (rawProgress) and the
        // best progress we've seen (bestRawProgress). As you are downloading, you may
        // encounter new assets that require downloads, increasing the number of
        // pending downloads and thus decreasing your overall progress.
        bestRawProgress = 0,

        // True if we have known active downloads
        isDownloading = false,

        // Entities are streamed to users, so you don't receive them all at once; instead, you
        // receive them over a period of time. In many cases we end up in a situation where
        //
        // The initial delay cooldown keeps us from tracking progress before the allotted time
        // has passed.
        INITIAL_DELAY_COOLDOWN_TIME = 1000,
        initialDelayCooldown = 0;

    function fade() {

        alpha = alpha + alphaDelta;

        if (alpha < 0) {
            alpha = 0;
        } else if (alpha > 1) {
            alpha = 1;
        }

        if (alpha === 0 || alpha === 1) { // Finished fading in or out
            alphaDelta = 0;
            Script.clearInterval(fadeTimer);
        }

        if (alpha === 0) { // Finished fading out
            visible = false;
        }

        Overlays.editOverlay(barDesktop.overlay, {
            alpha: alpha,
            visible: visible && !isHMD
        });
        Overlays.editOverlay(barHMD.overlay, {
            alpha: alpha,
            visible: visible && isHMD
        });
        Overlays.editOverlay(textDesktop.overlay, {
            alpha: alpha,
            visible: visible && !isHMD
        });
        Overlays.editOverlay(textHMD.overlay, {
            alpha: alpha,
            visible: visible && isHMD
        });
    }

    Window.domainChanged.connect(function () {
        isDownloading = false;
        bestRawProgress = 100;
        rawProgress = 100;
        displayProgress = 100;
    });

    function onDownloadInfoChanged(info) {

        debug("PROGRESS: Download info changed ", info.downloading.length, info.pending, maxSeen);

        // Update raw progress value
        if (info.downloading.length + info.pending === 0) {
            isDownloading = false;
            rawProgress = 100;
            bestRawProgress = 100;
            initialDelayCooldown = INITIAL_DELAY_COOLDOWN_TIME;
        } else {
            var count = info.downloading.length + info.pending;
            if (!isDownloading) {
                isDownloading = true;
                bestRawProgress = 0;
                rawProgress = 0;
                initialDelayCooldown = INITIAL_DELAY_COOLDOWN_TIME;
                displayProgress = 0;
                maxSeen = count;
            }
            if (count > maxSeen) {
                maxSeen = count;
            }
            if (initialDelayCooldown <= 0) {
                rawProgress = ((maxSeen - count) / maxSeen) * 100;

                if (rawProgress > bestRawProgress) {
                    bestRawProgress = rawProgress;
                }
            }
        }
        debug("PROGRESS:", rawProgress, bestRawProgress, maxSeen);
    }

    function createOverlays() {
        barDesktop.overlay = Overlays.addOverlay("image", {
            imageURL: BAR_DESKTOP_URL,
            subImage: {
                x: 0,
                y: 0,
                width: BAR_DESKTOP_WIDTH - BAR_DESKTOP_REPEAT,
                height: BAR_DESKTOP_HEIGHT
            },
            width: barDesktop.width,
            height: barDesktop.height,
            visible: false,
            alpha: 0.0
        });
        barHMD.overlay = Overlays.addOverlay("image", {
            imageURL: BAR_HMD_URL,
            subImage: {
                x: 0,
                y: 0,
                width: BAR_HMD_WIDTH - BAR_HMD_REPEAT,
                height: BAR_HMD_HEIGHT
            },
            width: barHMD.width,
            height: barHMD.height,
            visible: false,
            alpha: 0.0
        });
        textDesktop.overlay = Overlays.addOverlay("image", {
            imageURL: TEXT_URL,
            width: textDesktop.width,
            height: textDesktop.height,
            visible: false,
            alpha: 0.0
        });
        textHMD.overlay = Overlays.addOverlay("image", {
            imageURL: TEXT_URL,
            width: textHMD.width,
            height: textHMD.height,
            visible: false,
            alpha: 0.0
        });
    }

    function deleteOverlays() {
        Overlays.deleteOverlay(barDesktop.overlay);
        Overlays.deleteOverlay(barHMD.overlay);
        Overlays.deleteOverlay(textDesktop.overlay);
        Overlays.deleteOverlay(textHMD.overlay);
    }

    function updateProgressBarLocation() {
        var viewport = Controller.getViewportDimensions();

        windowWidth = viewport.x;
        windowHeight = viewport.y;
        isHMD = HMD.active;

        if (isHMD) {
            barHMD.width = windowWidth;

            Overlays.editOverlay(barHMD.overlay, {
                x: windowWidth / 2 - barHMD.width / 2,
                y: windowHeight - 2 * barHMD.height + BAR_Y_OFFSET_HMD,
                width: barHMD.width
            });

            Overlays.editOverlay(textHMD.overlay, {
                x: windowWidth / 2 - textHMD.width / 2,
                y: windowHeight - 2 * textHMD.height + BAR_Y_OFFSET_HMD
            });
        } else {
            barDesktop.width = windowWidth;

            Overlays.editOverlay(barDesktop.overlay, {
                x: windowWidth / 2 - barDesktop.width / 2,
                y: windowHeight - 2 * barDesktop.height + BAR_Y_OFFSET_2D,
                width: barDesktop.width
            });

            Overlays.editOverlay(textDesktop.overlay, {
                x: windowWidth / 2 - textDesktop.width / 2,
                y: windowHeight - 2 * textDesktop.height + BAR_Y_OFFSET_2D
            });
        }
    }

    function update() {
        var viewport, diff;

        initialDelayCooldown -= 30;

        if (displayProgress < rawProgress) {
            diff = rawProgress - displayProgress;
            if (diff < 0.5) {
                displayProgress = rawProgress;
            } else {
                displayProgress += diff * 0.05;
            }
        }

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
                    fadeWaitTimer = Script.setTimeout(function () {
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
            Overlays.editOverlay(barDesktop.overlay, {
                visible: !isHMD,
                subImage: {
                    x: BAR_DESKTOP_REPEAT * (1 - displayProgress / 100),
                    y: 0,
                    width: BAR_DESKTOP_WIDTH - BAR_DESKTOP_REPEAT,
                    height: BAR_DESKTOP_HEIGHT
                }
            });

            Overlays.editOverlay(barHMD.overlay, {
                visible: isHMD,
                subImage: {
                    x: BAR_HMD_REPEAT * (1 - displayProgress / 100),
                    y: 0,
                    width: BAR_HMD_WIDTH - BAR_HMD_REPEAT,
                    height: BAR_HMD_HEIGHT
                }
            });

            Overlays.editOverlay(textDesktop.overlay, {
                visible: !isHMD
            });

            Overlays.editOverlay(textHMD.overlay, {
                visible: isHMD
            });

            // Update 2D overlays to maintain positions at bottom middle of window
            viewport = Controller.getViewportDimensions();

            if (viewport.x !== windowWidth || viewport.y !== windowHeight || isHMD !== HMD.active) {
                updateProgressBarLocation();
            }
        }
    }

    function setUp() {
        isHMD = HMD.active;

        barDesktop.width = BAR_DESKTOP_WIDTH;
        barDesktop.height = BAR_DESKTOP_HEIGHT;
        barHMD.width = BAR_HMD_WIDTH;
        barHMD.height = BAR_HMD_HEIGHT;

        textDesktop.width = SCALE_TEXT_DESKTOP * TEXT_WIDTH;
        textDesktop.height = SCALE_TEXT_DESKTOP * TEXT_HEIGHT;
        textHMD.width = SCALE_TEXT_HMD * TEXT_WIDTH;
        textHMD.height = SCALE_TEXT_HMD * TEXT_HEIGHT;

        createOverlays();
    }

    function tearDown() {
        deleteOverlays();
    }

    setUp();
    GlobalServices.downloadInfoChanged.connect(onDownloadInfoChanged);
    GlobalServices.updateDownloadInfo();
    Script.setInterval(update, 1000 / 60);
    Script.scriptEnding.connect(tearDown);

}()); // END LOCAL_SCOPE
