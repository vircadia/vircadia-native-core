//
//  marketplace.js
//
//  Created by Eric Levin on 8 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global WebTablet */
Script.include("./libraries/WebTablet.js");

var toolIconUrl = Script.resolvePath("assets/images/tools/");

var MARKETPLACE_URL = "https://metaverse.highfidelity.com/marketplace";
var marketplaceWindow = new OverlayWebWindow({
    title: "Marketplace",
    source: "about:blank",
    width: 900,
    height: 700,
    visible: false
});

var toolHeight = 50;
var toolWidth = 50;
var TOOLBAR_MARGIN_Y = 0;
var marketplaceVisible = false;
var marketplaceWebTablet;

function shouldShowWebTablet() {
    var rightPose = Controller.getPoseValue(Controller.Standard.RightHand);
    var leftPose = Controller.getPoseValue(Controller.Standard.LeftHand);
    return HMD.active && (leftPose.valid || rightPose.valid);
}

function showMarketplace(marketplaceID) {
    if (shouldShowWebTablet()) {
        marketplaceWebTablet = new WebTablet("https://metaverse.highfidelity.com/marketplace");
    } else {
        var url = MARKETPLACE_URL;
        if (marketplaceID) {
            url = url + "/items/" + marketplaceID;
        }
        marketplaceWindow.setURL(url);
        marketplaceWindow.setVisible(true);
    }

    marketplaceVisible = true;
    UserActivityLogger.openedMarketplace();
}

function hideMarketplace() {
    if (marketplaceWindow.visible) {
        marketplaceWindow.setVisible(false);
        marketplaceWindow.setURL("about:blank");
    } else {
        marketplaceWebTablet.destroy();
        marketplaceWebTablet = null;
    }
    marketplaceVisible = false;
}

function toggleMarketplace() {
    if (marketplaceVisible) {
        hideMarketplace();
    } else {
        showMarketplace();
    }
}

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

var browseExamplesButton = toolBar.addButton({
    imageURL: toolIconUrl + "market.svg",
    objectName: "marketplace",
    buttonState: 1,
    defaultState: 1,
    hoverState: 3,
    alpha: 0.9
});

function onExamplesWindowVisibilityChanged() {
    browseExamplesButton.writeProperty('buttonState', marketplaceWindow.visible ? 0 : 1);
    browseExamplesButton.writeProperty('defaultState', marketplaceWindow.visible ? 0 : 1);
    browseExamplesButton.writeProperty('hoverState', marketplaceWindow.visible ? 2 : 3);
}
function onClick() {
    toggleMarketplace();
}
browseExamplesButton.clicked.connect(onClick);
marketplaceWindow.visibleChanged.connect(onExamplesWindowVisibilityChanged);

Script.scriptEnding.connect(function () {
    toolBar.removeButton("marketplace");
    browseExamplesButton.clicked.disconnect(onClick);
    marketplaceWindow.visibleChanged.disconnect(onExamplesWindowVisibilityChanged);
});
