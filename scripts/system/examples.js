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

//var EXAMPLES_URL = "https://metaverse.highfidelity.com/examples";
//var EXAMPLES_URL = "https://clara.io/view/c1c4d926-5648-4fd3-9673-6d9018ad4627";
var EXAMPLES_URL = "https://clara.io/library";
//var EXAMPLES_URL = "http://s3.amazonaws.com/DreamingContent/test.html";
//var EXAMPLES_URL = "https://hifi-content.s3.amazonaws.com/elisalj/test.html";
//var EXAMPLES_URL = "https://hifi-content.s3.amazonaws.com/elisalj/test_download.html";

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
    alpha: 0.9
});

function onExamplesWindowVisibilityChanged() {
    browseExamplesButton.writeProperty('buttonState', examplesWindow.visible ? 0 : 1);
}
function onClick() {
    toggleExamples();
}
browseExamplesButton.clicked.connect(onClick);
examplesWindow.visibleChanged.connect(onExamplesWindowVisibilityChanged);

Script.scriptEnding.connect(function () {
    browseExamplesButton.clicked.disconnect(onClick);
    examplesWindow.visibleChanged.disconnect(onExamplesWindowVisibilityChanged);
});
