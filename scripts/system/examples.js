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
var TOOLBAR_MARGIN_Y = 0;


function showExamples(marketplaceID) {
    var url = EXAMPLES_URL;
    if (marketplaceID) {
        url = url + "/items/" + marketplaceID;
    }
    print("setting examples URL to " + url);
    examplesWindow.setURL(url);
    examplesWindow.setVisible(true);

    UserActivityLogger.openedMarketplace();
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

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

var browseExamplesButton = toolBar.addButton({
    imageURL: toolIconUrl + "market.svg",
    objectName: "examples",
    buttonState: 1,
    defaultState: 1,
    hoverState: 3,
    alpha: 0.9
});

function onExamplesWindowVisibilityChanged() {
    browseExamplesButton.writeProperty('buttonState', examplesWindow.visible ? 0 : 1);
    browseExamplesButton.writeProperty('defaultState', examplesWindow.visible ? 0 : 1);
    browseExamplesButton.writeProperty('hoverState', examplesWindow.visible ? 2 : 3);
}
function onClick() {
    toggleExamples();
}
browseExamplesButton.clicked.connect(onClick);
examplesWindow.visibleChanged.connect(onExamplesWindowVisibilityChanged);

Script.scriptEnding.connect(function () {
    toolBar.removeButton("examples");
    browseExamplesButton.clicked.disconnect(onClick);
    examplesWindow.visibleChanged.disconnect(onExamplesWindowVisibilityChanged);
});
