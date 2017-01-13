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
var CLARA_IO_STATUS = "CLARA.IO STATUS";
var CLARA_IO_CANCEL_DOWNLOAD = "CLARA.IO CANCEL DOWNLOAD";
var CLARA_IO_CANCELLED_DOWNLOAD = "CLARA.IO CANCELLED DOWNLOAD";
var GOTO_DIRECTORY = "GOTO_DIRECTORY";
var QUERY_CAN_WRITE_ASSETS = "QUERY_CAN_WRITE_ASSETS";
var CAN_WRITE_ASSETS = "CAN_WRITE_ASSETS";
var WARN_USER_NO_PERMISSIONS = "WARN_USER_NO_PERMISSIONS";

var CLARA_DOWNLOAD_TITLE = "Preparing Download";
var messageBox = null;
var isDownloadBeingCancelled = false;

var CANCEL_BUTTON = 4194304;  // QMessageBox::Cancel
var NO_BUTTON = 0;  // QMessageBox::NoButton

var NO_PERMISSIONS_ERROR_MESSAGE = "Cannot download model because you can't write to \nthe domain's Asset Server.";

var marketplaceWindow = new OverlayWebWindow({
    title: "Marketplace",
    source: "about:blank",
    width: 900,
    height: 700,
    visible: false
});
marketplaceWindow.setScriptURL(MARKETPLACES_INJECT_SCRIPT_URL);

function onWebEventReceived(message) {
    if (message === GOTO_DIRECTORY) {
        var url = MARKETPLACES_URL;
        if (marketplaceWindow.visible) {
            marketplaceWindow.setURL(url);
        }
        if (marketplaceWebTablet) {
            marketplaceWebTablet.setURL(url);
        }
        return;
    }
    if (message === QUERY_CAN_WRITE_ASSETS) {
        var canWriteAssets = CAN_WRITE_ASSETS + " " + Entities.canWriteAssets();
        if (marketplaceWindow.visible) {
            marketplaceWindow.emitScriptEvent(canWriteAssets);
        }
        if (marketplaceWebTablet) {
            marketplaceWebTablet.getOverlayObject().emitScriptEvent(canWriteAssets);
        }
        return;
    }
    if (message === WARN_USER_NO_PERMISSIONS) {
        Window.alert(NO_PERMISSIONS_ERROR_MESSAGE);
        return;
    }

    if (message.slice(0, CLARA_IO_STATUS.length) === CLARA_IO_STATUS) {
        if (isDownloadBeingCancelled) {
            return;
        }

        var text = message.slice(CLARA_IO_STATUS.length);
        if (messageBox === null) {
            messageBox = Window.openMessageBox(CLARA_DOWNLOAD_TITLE, text, CANCEL_BUTTON, NO_BUTTON);
        } else {
            Window.updateMessageBox(messageBox, CLARA_DOWNLOAD_TITLE, text, CANCEL_BUTTON, NO_BUTTON);
        }
        return;
    }

    if (message.slice(0, CLARA_IO_DOWNLOAD.length) === CLARA_IO_DOWNLOAD) {
        if (messageBox !== null) {
            Window.closeMessageBox(messageBox);
            messageBox = null;
        }
        return;
    }

    if (message === CLARA_IO_CANCELLED_DOWNLOAD) {
        isDownloadBeingCancelled = false;
    }
}

marketplaceWindow.webEventReceived.connect(onWebEventReceived);

function onMessageBoxClosed(id, button) {
    if (id === messageBox && button === CANCEL_BUTTON) {
        isDownloadBeingCancelled = true;
        messageBox = null;
        marketplaceWindow.emitScriptEvent(CLARA_IO_CANCEL_DOWNLOAD);
    }
}

Window.messageBoxClosed.connect(onMessageBoxClosed);

var toolHeight = 50;
var toolWidth = 50;
var TOOLBAR_MARGIN_Y = 0;
var marketplaceVisible = false;
var marketplaceWebTablet;

// We persist clientOnly data in the .ini file, and reconstitute it on restart.
// To keep things consistent, we pickle the tablet data in Settings, and kill any existing such on restart and domain change.
var persistenceKey = "io.highfidelity.lastDomainTablet";

function showMarketplace() {
    if (shouldShowWebTablet()) {
        updateButtonState(true);
        marketplaceWebTablet = new WebTablet(MARKETPLACE_URL_INITIAL, null, null, true);
        Settings.setValue(persistenceKey, marketplaceWebTablet.pickle());
        marketplaceWebTablet.setScriptURL(MARKETPLACES_INJECT_SCRIPT_URL);
        marketplaceWebTablet.getOverlayObject().webEventReceived.connect(onWebEventReceived);
    } else {
        marketplaceWindow.setURL(MARKETPLACE_URL_INITIAL);
        marketplaceWindow.setVisible(true);
    }

    marketplaceVisible = true;
    UserActivityLogger.openedMarketplace();

    // FIXME - the code to support the following is not yet implented
    /*tablet.setScriptURL(MARKETPLACES_INJECT_SCRIPT_URL);
    tablet.webEventRecieved.connect(function (message) {
       if (message === GOTO_DIRECTORY) {
           tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL);
       }

      if (message === QUERY_CAN_WRITE_ASSESTS) {
          tablet.emitScriptEvent(CAN_WRITE_ASSETS + " " + Entities.canWriteAssets());
      }

      if (message === WARN_USER_NO_PERMISSIONS) {
          Window.alert(NO_PERMISSIONS_ERROR_MESSAGE);
      }

    });*/
}

function toggleMarketplace() {
    showMarketplace();
}

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

var browseExamplesButton = tablet.addButton({
    icon: "icons/tablet-icons/market-i.svg",
    text: "MARKET"
});

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
Entities.canWriteAssetsChanged.connect(onCanWriteAssetsChanged);

Script.scriptEnding.connect(function () {
    browseExamplesButton.clicked.disconnect(onClick);
    tablet.removeButton(browseExamplesButton);
    marketplaceWindow.visibleChanged.disconnect(onMarketplaceWindowVisibilityChanged);
    Entities.canWriteAssetsChanged.disconnect(onCanWriteAssetsChanged);
});

}()); // END LOCAL_SCOPE
