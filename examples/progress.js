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

    var progress = 100,  // %
        alpha = 0.0,
        alphaDelta = 0.0,  // > 0 if fading in; < 0 if fading out
        ALPHA_DELTA_IN = 0.15,
        ALPHA_DELTA_OUT = -0.02,
        fadeTimer = null,
        FADE_INTERVAL = 30,  // ms between changes in alpha
        fadeWaitTimer = null,
        FADE_OUT_WAIT = 1000,  // Wait before starting to fade out after progress 100%
        visible = false,
        BAR_COLOR = { red: 0, green: 128, blue: 0 },
        BAR_WIDTH = 320,  // Nominal dimension of SVG in pixels
        BAR_HEIGHT = 20,
        BAR_URL = "http://ctrlaltstudio.com/hifi/progress-bar.svg",
        BACKGROUND_WIDTH = 360,
        BACKGROUND_HEIGHT = 60,
        BACKGROUND_URL = "http://ctrlaltstudio.com/hifi/progress-bar-background.svg",
        SCALE_2D = 0.5,  // Scale the SVGs for 2D display
        background2D = {},
        bar2D = {},
        windowWidth = 0,
        windowHeight = 0;

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

        Overlays.editOverlay(background2D.overlay, {
            alpha: alpha,
            visible: visible
        });
        Overlays.editOverlay(bar2D.overlay, {
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
            Overlays.editOverlay(bar2D.overlay, {
                width: progress / 100 * bar2D.width
            });
        }
    }

    function update() {
        var viewport;

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

    function setUp() {
        background2D.width = SCALE_2D * BACKGROUND_WIDTH;
        background2D.height = SCALE_2D * BACKGROUND_HEIGHT;
        background2D.overlay = Overlays.addOverlay("image", {
            width: background2D.width,
            height: background2D.height,
            imageURL: BACKGROUND_URL,
            visible: false,
            alpha: 0.0
        });

        bar2D.width = SCALE_2D * BAR_WIDTH;
        bar2D.height = SCALE_2D * BAR_HEIGHT;
        bar2D.overlay = Overlays.addOverlay("image", {
            width: bar2D.width,
            height: bar2D.height,
            imageURL: BAR_URL,
            color: BAR_COLOR,
            visible: false,
            alpha: 0.0
        });
    }

    function tearDown() {
        Overlays.deleteOverlay(background2D.overlay);
        Overlays.deleteOverlay(bar2D.overlay);
    }

    setUp();
    GlobalServices.downloadInfoChanged.connect(onDownloadInfoChanged);
    GlobalServices.updateDownloadInfo();
    Script.update.connect(update);
    Script.scriptEnding.connect(tearDown);
}());
