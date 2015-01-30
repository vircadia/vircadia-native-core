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
        BAR_COLOR = { red: 0, green: 128, blue: 0 },
        BAR_WIDTH = 320,  // Raw dimension in pixels of SVG
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

    function updateInfo(info) {
        var i;

        if (info.downloading.length + info.pending === 0) {
            progress = 100;
        } else {
            progress = 0;
            for (i = 0; i < info.downloading.length; i += 1) {
                progress += info.downloading[i];
            }
            progress = progress / (info.downloading.length + info.pending);
        }

        Overlays.editOverlay(bar2D.overlay, {
            width: progress / 100 * bar2D.width
        });
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
            visible: true,
            alpha: 1.0
        });

        bar2D.width = SCALE_2D * BAR_WIDTH;
        bar2D.height = SCALE_2D * BAR_HEIGHT;
        bar2D.overlay = Overlays.addOverlay("image", {
            width: bar2D.width,
            height: bar2D.height,
            imageURL: BAR_URL,
            color: BAR_COLOR,
            visible: true,
            alpha: 1.0
        });
    }

    function tearDown() {
        Overlays.deleteOverlay(background2D.overlay);
        Overlays.deleteOverlay(bar2D.overlay);
    }

    setUp();
    GlobalServices.downloadInfoChanged.connect(updateInfo);
    GlobalServices.updateDownloadInfo();
    Script.update.connect(update);
    Script.scriptEnding.connect(tearDown);
}());
