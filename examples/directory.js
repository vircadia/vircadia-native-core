//
//  directory.js
//  examples
//
//  Created by David Rowe on 8 Jun 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var directory = (function () {

    var DIRECTORY_URL = "https://metaverse.highfidelity.com/directory",
        directoryWindow,
        DIRECTORY_BUTTON_URL = HIFI_PUBLIC_BUCKET + "images/tools/directory.svg",
        BUTTON_WIDTH = 50,
        BUTTON_HEIGHT = 50,
        BUTTON_ALPHA = 0.9,
        BUTTON_MARGIN = 8,
        directoryButton,
        EDIT_TOOLBAR_BUTTONS = 10,  // Number of buttons in edit.js toolbar
        viewport;

    function updateButtonPosition() {
        Overlays.editOverlay(directoryButton, {
            x: viewport.x - BUTTON_WIDTH - BUTTON_MARGIN,
            y: (viewport.y - (EDIT_TOOLBAR_BUTTONS + 1) * (BUTTON_HEIGHT + BUTTON_MARGIN) - BUTTON_MARGIN) / 2 - 1
        });
    }

    function onMousePressEvent(event) {
        var clickedOverlay;

        clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (clickedOverlay === directoryButton) {
            if (directoryWindow.url !== DIRECTORY_URL) {
                directoryWindow.setURL(DIRECTORY_URL);
            }
            directoryWindow.setVisible(true);
            directoryWindow.raise();
        }
    }

    function onDomainChanged() {
        directoryWindow.setVisible(false);
    }

    function onScriptUpdate() {
        var oldViewport = viewport;

        viewport = Controller.getViewportDimensions();

        if (viewport.x !== oldViewport.x || viewport.y !== oldViewport.y) {
            updateButtonPosition();
        }
    }

    function setUp() {
        viewport = Controller.getViewportDimensions();

        directoryWindow = new OverlayWebWindow({
            title: 'Directory', 
            source: DIRECTORY_URL, 
            width: 900, 
            height: 700,
            visible: false      
        });

        directoryButton = Overlays.addOverlay("image", {
            imageURL: DIRECTORY_BUTTON_URL,
            width: BUTTON_WIDTH,
            height: BUTTON_HEIGHT,
            x: viewport.x - BUTTON_WIDTH - BUTTON_MARGIN,
            y: BUTTON_MARGIN,
            alpha: BUTTON_ALPHA,
            visible: true
        });

        updateButtonPosition();

        Controller.mousePressEvent.connect(onMousePressEvent);
        Window.domainChanged.connect(onDomainChanged);

        Script.update.connect(onScriptUpdate);
    }

    function tearDown() {
        Overlays.deleteOverlay(directoryButton);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());