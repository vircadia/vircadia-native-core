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
        BAR_WIDTH = 480, // Dimension of SVG in pixels of visible portion (half) of the bar.
        BAR_HEIGHT = 10,
        BAR_Y_OFFSET_2D = -10, // Offset of progress bar while in desktop mode
        BAR_Y_OFFSET_HMD = -300, // Offset of progress bar while in HMD
        BAR_URL = Script.resolvePath("assets/images/progress-bar.svg"),
        TEXT_HEIGHT = 32,
        TEXT_WIDTH = 256,
        TEXT_URL = Script.resolvePath("assets/images/progress-bar-text.svg"),
        BACKGROUND_WIDTH = 520,
        BACKGROUND_HEIGHT = 50,
        BACKGROUND_URL = Script.resolvePath("assets/images/progress-bar-background.svg"),
        windowWidth = 0,
        windowHeight = 0,
        bar2D = {},
        background2D = {},
        textDesktop = {},  // Separate desktop and HMD overlays because can't change text size after overlay created.
        textHMD = {},
        SCALE_2D = 0.35, // Scale the SVGs for 2D display.
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

        Overlays.editOverlay(background2D.overlay, {
            alpha: alpha,
            visible: visible
        });
        Overlays.editOverlay(bar2D.overlay, {
            alpha: alpha,
            visible: visible
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
        Overlays.deleteOverlay(background2D.overlay);
        Overlays.deleteOverlay(bar2D.overlay);
    }

    function updateProgressBarLocation() {
        var viewport = Controller.getViewportDimensions(),
            yOffset;

        windowWidth = viewport.x;
        windowHeight = viewport.y;
        isHMD = HMD.active;
        yOffset = isHMD ? BAR_Y_OFFSET_HMD : BAR_Y_OFFSET_2D;

        Overlays.editOverlay(background2D.overlay, {
            x: windowWidth / 2 - background2D.width / 2,
            y: windowHeight - background2D.height - bar2D.height + yOffset
        });

        Overlays.editOverlay(bar2D.overlay, {
            x: windowWidth / 2 - bar2D.width / 2,
            y: windowHeight - background2D.height - bar2D.height + (background2D.height - bar2D.height) / 2 + yOffset
        });

        Overlays.editOverlay(textDesktop.overlay, {
            x: windowWidth / 2 - textDesktop.width / 2,
            y: windowHeight - 2 * textDesktop.height + yOffset
        });

        Overlays.editOverlay(textHMD.overlay, {
            x: windowWidth / 2 - textHMD.width / 2,
            y: windowHeight - 2 * textHMD.height + yOffset
        });
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
            Overlays.editOverlay(bar2D.overlay, {
                visible: true,
                subImage: {

                    // $$$$$$$

                    x: BAR_WIDTH * (1 - displayProgress / 100),
                    y: 0,
                    width: BAR_WIDTH,
                    height: BAR_HEIGHT
                }
            });

            Overlays.editOverlay(background2D.overlay, {
                visible: true
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

        background2D.width = SCALE_2D * BACKGROUND_WIDTH;
        background2D.height = SCALE_2D * BACKGROUND_HEIGHT;
        bar2D.width = SCALE_2D * BAR_WIDTH;
        bar2D.height = SCALE_2D * BAR_HEIGHT;
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
