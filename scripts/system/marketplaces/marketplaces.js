//
//  marketplaces.js
//
//  Created by Eric Levin on 8 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Tablet, Script, HMD, UserActivityLogger, Entities */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

Script.include("../libraries/WebTablet.js");

var MARKETPLACE_URL = "https://metaverse.highfidelity.com/marketplace";
var MARKETPLACE_URL_INITIAL = MARKETPLACE_URL + "?";  // Append "?" to signal injected script that it's the initial page.
var MARKETPLACES_URL = Script.resolvePath("../html/marketplaces.html");
var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");

var HOME_BUTTON_TEXTURE = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";
// var HOME_BUTTON_TEXTURE = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";

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

function onMessageBoxClosed(id, button) {
    if (id === messageBox && button === CANCEL_BUTTON) {
        isDownloadBeingCancelled = true;
        messageBox = null;
        tablet.emitScriptEvent(CLARA_IO_CANCEL_DOWNLOAD);
    }
}

Window.messageBoxClosed.connect(onMessageBoxClosed);

function showMarketplace() {
    UserActivityLogger.openedMarketplace();

    tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
    tablet.webEventReceived.connect(function (message) {

        if (message === GOTO_DIRECTORY) {
            tablet.gotoWebScreen(MARKETPLACES_URL, MARKETPLACES_INJECT_SCRIPT_URL);
        }

        if (message === QUERY_CAN_WRITE_ASSETS) {
            tablet.emitScriptEvent(CAN_WRITE_ASSETS + " " + Entities.canWriteAssets());
        }

        if (message === WARN_USER_NO_PERMISSIONS) {
            Window.alert(NO_PERMISSIONS_ERROR_MESSAGE);
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
    });
}

function toggleMarketplace() {
    var entity = HMD.tabletID;
    Entities.editEntity(entity, {textures: JSON.stringify({"tex.close": HOME_BUTTON_TEXTURE})});
    showMarketplace();
}

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var marketplaceButton = tablet.addButton({
    icon: "icons/tablet-icons/market-i.svg",
    text: "MARKET",
    sortOrder: 9
});

function onCanWriteAssetsChanged() {
    var message = CAN_WRITE_ASSETS + " " + Entities.canWriteAssets();
    tablet.emitScriptEvent(message);
}

function onClick() {
    toggleMarketplace();
}

marketplaceButton.clicked.connect(onClick);
Entities.canWriteAssetsChanged.connect(onCanWriteAssetsChanged);

Script.scriptEnding.connect(function () {
    if (tablet) {
        tablet.removeButton(marketplaceButton);
    }
    Entities.canWriteAssetsChanged.disconnect(onCanWriteAssetsChanged);
});

}()); // END LOCAL_SCOPE
