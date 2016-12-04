//
//  marketplaces.js
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

var MARKETPLACES_URL = Script.resolvePath("../html/marketplaces.html");
var MARKETPLACES_DIRECTORY_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesDirectory.js");
var MARKETPLACES_HFIF_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesHiFi.js");
var MARKETPLACES_CLARA_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesClara.js");

var marketplaceWindow = new OverlayWebWindow({
    title: "Marketplace",
    source: "about:blank",
    width: 900,
    height: 700,
    visible: false
});
marketplaceWindow.setScriptURL(MARKETPLACES_DIRECTORY_SCRIPT_URL);

marketplaceWindow.webEventReceived.connect(function (data) {
    if (data === "INJECT_CLARA") {
        marketplaceWindow.setScriptURL(MARKETPLACES_CLARA_SCRIPT_URL);
    }
    if (data === "INJECT_HIFI") {
        marketplaceWindow.setScriptURL(MARKETPLACES_HFIF_SCRIPT_URL);
    }
    if (data === "RELOAD_DIRECTORY") {
        marketplaceWindow.setScriptURL(MARKETPLACES_DIRECTORY_SCRIPT_URL);
        marketplaceWindow.setURL(MARKETPLACES_URL);
    }
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
    if (shouldShowWebTablet()) {
        updateButtonState(true);
        marketplaceWebTablet = new WebTablet(MARKETPLACES_URL, null, null, true);
        Settings.setValue(persistenceKey, marketplaceWebTablet.pickle());
        marketplaceWebTablet.setScriptURL(MARKETPLACES_DIRECTORY_SCRIPT_URL);
    } else {
        var url = MARKETPLACES_URL;
        if (marketplaceID) {
            // $$$$$$$ TODO
            url = url + "/items/" + marketplaceID;
        }
        marketplaceWindow.setURL(url);
        marketplaceWindow.setVisible(true);
    }

    marketplaceVisible = true;
    UserActivityLogger.openedMarketplace();
}

function hideTablet(tablet) {
    if (!tablet) {
        return;
    }
    updateButtonState(false);
    tablet.destroy();
    marketplaceWebTablet = null;
    Settings.setValue(persistenceKey, "");
}
function clearOldTablet() { // If there was a tablet from previous domain or session, kill it and let it be recreated
    var tablet = WebTablet.unpickle(Settings.getValue(persistenceKey, ""));
    hideTablet(tablet);
}
function hideMarketplace() {
    if (marketplaceWindow.visible) {
        marketplaceWindow.setVisible(false);
        marketplaceWindow.setURL("about:blank");
    } else if (marketplaceWebTablet) {
        hideTablet(marketplaceWebTablet);
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

function updateButtonState(visible) {
    browseExamplesButton.writeProperty('buttonState', visible ? 0 : 1);
    browseExamplesButton.writeProperty('defaultState', visible ? 0 : 1);
    browseExamplesButton.writeProperty('hoverState', visible ? 2 : 3);
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
    toolBar.removeButton("marketplace");
    browseExamplesButton.clicked.disconnect(onClick);
    marketplaceWindow.visibleChanged.disconnect(onMarketplaceWindowVisibilityChanged);
});

}()); // END LOCAL_SCOPE
