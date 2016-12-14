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

var MARKETPLACE_URL = "https://metaverse.highfidelity.com/marketplace";
var MARKETPLACE_URL_INITIAL = MARKETPLACE_URL + "?";  // Append "?" to signal injected script that it's the initial page.
var MARKETPLACES_URL = Script.resolvePath("../html/marketplaces.html");
var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");

// Event bridge messages.
var CLARA_IO_DOWNLOAD = "CLARA.IO DOWNLOAD";
var GOTO_DIRECTORY = "GOTO_DIRECTORY";
var QUERY_CAN_WRITE_ASSETS = "QUERY_CAN_WRITE_ASSETS";
var CAN_WRITE_ASSETS = "CAN_WRITE_ASSETS";
var WARN_USER_NO_PERMISSIONS = "WARN_USER_NO_PERMISSIONS";
var NO_PERMISSIONS_ERROR_MESSAGE = "Cannot download model because you can't write to \nthe domain's Asset Server.";

var marketplaceWindow = new OverlayWebWindow({
    title: "Marketplace",
    source: "about:blank",
    width: 900,
    height: 700,
    visible: false
});
marketplaceWindow.setScriptURL(MARKETPLACES_INJECT_SCRIPT_URL);
marketplaceWindow.webEventReceived.connect(function (message) {
    if (message === GOTO_DIRECTORY) {
        marketplaceWindow.setURL(MARKETPLACES_URL);
    }
    if (message === QUERY_CAN_WRITE_ASSETS) {
        marketplaceWindow.emitScriptEvent(CAN_WRITE_ASSETS + " " + Entities.canWriteAssets());
    }
    if (message === WARN_USER_NO_PERMISSIONS) {
        Window.alert(NO_PERMISSIONS_ERROR_MESSAGE);
    }
});

var toolHeight = 50;
var toolWidth = 50;
var TOOLBAR_MARGIN_Y = 0;
var marketplaceVisible = false;
var marketplaceWebTablet;

// We persist clientOnly data in the .ini file, and reconstitute it on restart.
// To keep things consistent, we pickle the tablet data in Settings, and kill any existing such on restart and domain change.
var persistenceKey = "io.highfidelity.lastDomainTablet";

function shouldShowWebTablet() {
    var rightPose = Controller.getPoseValue(Controller.Standard.RightHand);
    var leftPose = Controller.getPoseValue(Controller.Standard.LeftHand);
    var hasHydra = !!Controller.Hardware.Hydra;
    return HMD.active && (leftPose.valid || rightPose.valid || hasHydra);
}

function showMarketplace() {
    if (shouldShowWebTablet()) {
        updateButtonState(true);
        marketplaceWebTablet = new WebTablet(MARKETPLACE_URL_INITIAL, null, null, true);
        Settings.setValue(persistenceKey, marketplaceWebTablet.pickle());
        marketplaceWebTablet.setScriptURL(MARKETPLACES_INJECT_SCRIPT_URL);
        marketplaceWebTablet.getOverlayObject().webEventReceived.connect(function (message) {
            if (message === GOTO_DIRECTORY) {
                marketplaceWebTablet.setURL(MARKETPLACES_URL);
            }
            if (message === QUERY_CAN_WRITE_ASSETS) {
                marketplaceWebTablet.getOverlayObject().emitScriptEvent(CAN_WRITE_ASSETS + " " + Entities.canWriteAssets());
            }
            if (message === WARN_USER_NO_PERMISSIONS) {
                Window.alert(NO_PERMISSIONS_ERROR_MESSAGE);
            }
        });
    } else {
        marketplaceWindow.setURL(MARKETPLACE_URL_INITIAL);
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

function onCanWriteAssetsChanged() {
    var message = CAN_WRITE_ASSETS + " " + Entities.canWriteAssets();
    if (marketplaceWindow.visible) {
        marketplaceWindow.emitScriptEvent(message);
    }
    if (marketplaceWebTablet) {
        marketplaceWebTablet.getOverlayObject().emitScriptEvent(message);
    }
}

function onClick() {
    toggleMarketplace();
}

browseExamplesButton.clicked.connect(onClick);
marketplaceWindow.visibleChanged.connect(onMarketplaceWindowVisibilityChanged);
Entities.canWriteAssetsChanged.connect(onCanWriteAssetsChanged);

clearOldTablet(); // Run once at startup, in case there's anything laying around from a crash.
// We could also optionally do something like Window.domainChanged.connect(function () {Script.setTimeout(clearOldTablet, 2000)}),
// but the HUD version stays around, so lets do the same.

Script.scriptEnding.connect(function () {
    toolBar.removeButton("marketplace");
    browseExamplesButton.clicked.disconnect(onClick);
    marketplaceWindow.visibleChanged.disconnect(onMarketplaceWindowVisibilityChanged);
    Entities.canWriteAssetsChanged.disconnect(onCanWriteAssetsChanged);
});

}()); // END LOCAL_SCOPE
