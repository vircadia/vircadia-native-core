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
    imageURL: toolIconUrl + "examples-01.svg",
    objectName: "examples",
    yOffset: 50,
    alpha: 0.9,
});

var browseExamplesButtonDown = false;

browseExamplesButton.clicked.connect(function(){
    toggleExamples();
});

Script.scriptEnding.connect(function () {
    browseExamplesButton.clicked.disconnect();
});
