//
//  marketplace.js
//
//  Created by Eric Levin on 8 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

/* global WebTablet */
Script.include("../libraries/WebTablet.js");

var toolIconUrl = Script.resolvePath("../assets/images/tools/");

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

// We persist clientOnly data in the .ini file, and reconsistitute it on restart.
// To keep things consistent, we pickle the tablet data in Settings, and kill any existing such on restart and domain change.
var persistenceKey = "io.highfidelity.lastDomainTablet";

function shouldShowWebTablet() {
    var rightPose = Controller.getPoseValue(Controller.Standard.RightHand);
    var leftPose = Controller.getPoseValue(Controller.Standard.LeftHand);
    var hasHydra = !!Controller.Hardware.Hydra;
    return HMD.active && (leftPose.valid || rightPose.valid || hasHydra);
}

function showMarketplace(marketplaceID) {
    var url = MARKETPLACE_URL;
    if (marketplaceID) {
        url = url + "/items/" + marketplaceID;
    }
    tablet.gotoWebScreen(url);
    marketplaceVisible = true;
    UserActivityLogger.openedMarketplace();
}

function hideTablet(tablet) {
    if (!tablet) {
        return;
    }
    updateButtonState(false);
    tablet.unregister();
    tablet.destroy();
    marketplaceWebTablet = null;
    Settings.setValue(persistenceKey, "");
}
function clearOldTablet() { // If there was a tablet from previous domain or session, kill it and let it be recreated

}
function hideMarketplace() {
    if (marketplaceWindow.visible) {
        marketplaceWindow.setVisible(false);
        marketplaceWindow.setURL("about:blank");
    } else if (marketplaceWebTablet) {

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

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

var browseExamplesButton = tablet.addButton({
    icon: "icons/tablet-icons/market-i.svg",
    text: "MARKET"
});

function updateButtonState(visible) {

}
function onMarketplaceWindowVisibilityChanged() {
    updateButtonState(marketplaceWindow.visible);
    marketplaceVisible = marketplaceWindow.visible;
}

function onClick() {
    toggleMarketplace();
}

browseExamplesButton.clicked.connect(onClick);
marketplaceWindow.visibleChanged.connect(onMarketplaceWindowVisibilityChanged);

clearOldTablet(); // Run once at startup, in case there's anything laying around from a crash.
// We could also optionally do something like Window.domainChanged.connect(function () {Script.setTimeout(clearOldTablet, 2000)}),
// but the HUD version stays around, so lets do the same.

Script.scriptEnding.connect(function () {
    browseExamplesButton.clicked.disconnect(onClick);
    tablet.removeButton(browseExamplesButton);
    marketplaceWindow.visibleChanged.disconnect(onMarketplaceWindowVisibilityChanged);
});

}()); // END LOCAL_SCOPE
