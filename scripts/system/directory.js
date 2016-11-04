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

Script.include([
    "libraries/toolBars.js",
]);

var toolIconUrl = Script.resolvePath("assets/images/tools/");

var DIRECTORY_WINDOW_URL = "https://metaverse.highfidelity.com/directory";
var directoryWindow = new OverlayWebWindow({
    title: 'Directory',
    source: "about:blank",
    width: 900,
    height: 700,
    visible: false
});

var toolHeight = 50;
var toolWidth = 50;
var TOOLBAR_MARGIN_Y = 0;


function showDirectory() {
    directoryWindow.setURL(DIRECTORY_WINDOW_URL);
    directoryWindow.setVisible(true);
}

function hideDirectory() {
    directoryWindow.setVisible(false);
    directoryWindow.setURL("about:blank");
}

function toggleDirectory() {
    if (directoryWindow.visible) {
        hideDirectory();
    } else {
        showDirectory();
    }
}

var toolBar = (function() {
    var that = {},
        toolBar,
        browseDirectoryButton;

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.directory.toolbar", function(windowDimensions, toolbar) {
            return {
                x: windowDimensions.x / 2,  
                y: windowDimensions.y
            };
        }, {
            x: -2 * toolWidth,
            y: -TOOLBAR_MARGIN_Y - toolHeight
        });
        browseDirectoryButton = toolBar.addTool({
            imageURL: toolIconUrl + "directory-01.svg",
            subImage: {
                x: 0,
                y: Tool.IMAGE_WIDTH,
                width: Tool.IMAGE_WIDTH,
                height: Tool.IMAGE_HEIGHT
            },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

        toolBar.showTool(browseDirectoryButton, true);
    }

    var browseDirectoryButtonDown = false;
    that.mousePressEvent = function(event) {
        var clickedOverlay,
            url,
            file;

        if (!event.isLeftButton) {
            // if another mouse button than left is pressed ignore it
            return false;
        }

        clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });



        if (browseDirectoryButton === toolBar.clicked(clickedOverlay)) {
            toggleDirectory();
            return true;
        }

        return false;
    };

    that.mouseReleaseEvent = function(event) {
        var handled = false;


        if (browseDirectoryButtonDown) {
            var clickedOverlay = Overlays.getOverlayAtPoint({
                x: event.x,
                y: event.y
            });
        }

        newModelButtonDown = false;
        browseDirectoryButtonDown = false;

        return handled;
    }

    that.cleanup = function() {
        toolBar.cleanup();
    };

    initialize();
    return that;
}());

Controller.mousePressEvent.connect(toolBar.mousePressEvent)
Script.scriptEnding.connect(toolBar.cleanup);
