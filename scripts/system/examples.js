//
//  examples.js
//  examples
//
//  Created by Eric Levin on 8 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include([
    "libraries/toolBars.js",
]);

var toolIconUrl = Script.resolvePath("assets/images/tools/");

var EXAMPLES_URL = "https://metaverse.highfidelity.com/examples";
var examplesWindow = new OverlayWebWindow({
    title: 'Examples',
    source: "about:blank",
    width: 900,
    height: 700,
    visible: false
});

var toolHeight = 50;
var toolWidth = 50;


function showExamples(marketplaceID) {
    var url = EXAMPLES_URL;
    if (marketplaceID) {
        url = url + "/items/" + marketplaceID;
    }
    print("setting examples URL to " + url);
    examplesWindow.setURL(url);
    examplesWindow.setVisible(true);
}

function hideExamples() {
    examplesWindow.setVisible(false);
    examplesWindow.setURL("about:blank");
}

function toggleExamples() {
    if (examplesWindow.visible) {
        hideExamples();
    } else {
        showExamples();
    }
}

var toolBar = (function() {
    var that = {},
        toolBar,
        browseExamplesButton;

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.VERTICAL, "highfidelity.examples.toolbar", function(windowDimensions, toolbar) {
            return {
                x: windowDimensions.x - 8 - toolbar.width,
                y: 135
            };
        });
        browseExamplesButton = toolBar.addTool({
            imageURL: toolIconUrl + "examples-01.svg",
            subImage: {
                x: 0,
                y: Tool.IMAGE_WIDTH,
                width: Tool.IMAGE_WIDTH,
                height: Tool.IMAGE_HEIGHT
            },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true,
            showButtonDown: true
        });

        toolBar.showTool(browseExamplesButton, true);
    }

    var browseExamplesButtonDown = false;
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

        if (browseExamplesButton === toolBar.clicked(clickedOverlay)) {
            toggleExamples();
            return true;
        }

        return false;
    };

    that.mouseReleaseEvent = function(event) {
        var handled = false;


        if (browseExamplesButtonDown) {
            var clickedOverlay = Overlays.getOverlayAtPoint({
                x: event.x,
                y: event.y
            });
        }

        newModelButtonDown = false;
        browseExamplesButtonDown = false;

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