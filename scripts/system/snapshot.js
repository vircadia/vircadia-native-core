//
// snapshot.js
//
// Created by David Kelly on 1 August 2016
// Copyright 2016 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Tablet, Script, HMD, Settings, DialogsManager, Menu, Reticle, OverlayWebWindow, Desktop, Account, MyAvatar */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

var SNAPSHOT_DELAY = 500; // 500ms
var FINISH_SOUND_DELAY = 350;
var resetOverlays;
var reticleVisible;
var clearOverlayWhenMoving;

var buttonName = "SNAP";
var buttonConnected = false;

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    icon: "icons/tablet-icons/snap-i.svg",
    text: buttonName,
    sortOrder: 5
});

function shouldOpenFeedAfterShare() {
    var persisted = Settings.getValue('openFeedAfterShare', true); // might answer true, false, "true", or "false"
    return persisted && (persisted !== 'false');
}
function showFeedWindow() {
    if ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
            || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar"))) {
        tablet.loadQMLSource("TabletAddressDialog.qml");
    } else {
         tablet.initialScreen("TabletAddressDialog.qml");
        HMD.openTablet();
    }
}

var outstanding;
var readyData;
var shareAfterLogin = false;
var snapshotToShareAfterLogin;     
function onMessage(message) {
    // Receives message from the html dialog via the qwebchannel EventBridge. This is complicated by the following:
    // 1. Although we can send POJOs, we cannot receive a toplevel object. (Arrays of POJOs are fine, though.)
    // 2. Although we currently use a single image, we would like to take snapshot, a selfie, a 360 etc. all at the
    //    same time, show the user all of them, and have the user deselect any that they do not want to share.
    //    So we'll ultimately be receiving a set of objects, perhaps with different post processing for each.
    message = JSON.parse(message);
    if (message.type !== "snapshot") {
        return;
    }

    var isLoggedIn;
    var needsLogin = false;
    switch (message.action) {
        case 'ready':  // Send it.
            tablet.emitScriptEvent(JSON.stringify({
                type: "snapshot",
                action: readyData
            }));
            outstanding = 0;
            break;
        case 'openSettings':
            if ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
                    || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar"))) {
                Desktop.show("hifi/dialogs/GeneralPreferencesDialog.qml", "General Preferences");
            } else {
                tablet.loadQMLOnTop("TabletGeneralPreferences.qml");
            }
            break;
        case 'setOpenFeedFalse':
            Settings.setValue('openFeedAfterShare', false);
            break;
        case 'setOpenFeedTrue':
            Settings.setValue('openFeedAfterShare', true);
            break;
        default:
            //tablet.webEventReceived.disconnect(onMessage);  // <<< It's probably this that's missing?!
            HMD.closeTablet();
            isLoggedIn = Account.isLoggedIn();
            message.action.forEach(function (submessage) {
                if (submessage.share && !isLoggedIn) {
                    needsLogin = true;
                    submessage.share = false;
                    shareAfterLogin = true;
                    snapshotToShareAfterLogin = {path: submessage.localPath, href: submessage.href};
                }
                if (submessage.share) {
                    print('sharing', submessage.localPath);
                    outstanding = true;
                    Window.shareSnapshot(submessage.localPath, submessage.href);
                } else {
                    print('not sharing', submessage.localPath);
                }
                
            });
            if (outstanding && shouldOpenFeedAfterShare()) {
                showFeedWindow();
                outstanding = false;
            }
            if (needsLogin) { // after the possible feed, so that the login is on top
                var isLoggedIn = Account.isLoggedIn();

                if (!isLoggedIn) {
                    if ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
                        || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar"))) {
                        Menu.triggerOption("Login / Sign Up");
                    } else {
                        tablet.loadQMLOnTop("../../dialogs/TabletLoginDialog.qml");
                        HMD.openTablet();
                    }
                }
            }
    }
}

var SNAPSHOT_REVIEW_URL = Script.resolvePath("html/SnapshotReview.html");
var isInSnapshotReview = false;
function confirmShare(data) {
    tablet.gotoWebScreen(SNAPSHOT_REVIEW_URL);
    readyData = data;
    tablet.webEventReceived.connect(onMessage);
    HMD.openTablet();
    isInSnapshotReview = true;
}

function snapshotShared(errorMessage) {
    if (!errorMessage) {
        print('snapshot uploaded and shared');
    } else {
        print(errorMessage);
    }
    if ((--outstanding <= 0) && shouldOpenFeedAfterShare()) {
        showFeedWindow();
    }
}
var href, domainId;
function onClicked() {
    // Raising the desktop for the share dialog at end will interact badly with clearOverlayWhenMoving.
    // Turn it off now, before we start futzing with things (and possibly moving).
    clearOverlayWhenMoving = MyAvatar.getClearOverlayWhenMoving(); // Do not use Settings. MyAvatar keeps a separate copy.
    MyAvatar.setClearOverlayWhenMoving(false);

    // We will record snapshots based on the starting location. That could change, e.g., when recording a .gif.
    // Even the domainId could change (e.g., if the user falls into a teleporter while recording).
    href = location.href;
    domainId = location.domainId;

    // update button states
    resetOverlays = Menu.isOptionChecked("Overlays"); // For completness. Certainly true if the button is visible to be clicke.
    reticleVisible = Reticle.visible;
    Reticle.visible = false;
    Window.snapshotTaken.connect(resetButtons);

    // hide overlays if they are on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", false);
    }

    // take snapshot (with no notification)
    Script.setTimeout(function () {
        HMD.closeTablet();
        Script.setTimeout(function () {
            Window.takeSnapshot(false, true, 1.91);
        }, SNAPSHOT_DELAY);
    }, FINISH_SOUND_DELAY);
}

function isDomainOpen(id) {
    var request = new XMLHttpRequest();
    var options = [
        'now=' + new Date().toISOString(),
        'include_actions=concurrency',
        'domain_id=' + id.slice(1, -1),
        'restriction=open,hifi' // If we're sharing, we're logged in
        // If we're here, protocol matches, and it is online
    ];
    var url = location.metaverseServerUrl + "/api/v1/user_stories?" + options.join('&');
    request.open("GET", url, false);
    request.send();
    if (request.status !== 200) {
        return false;
    }
    var response = JSON.parse(request.response); // Not parsed for us.
    return (response.status === 'success') &&
        response.total_entries;
}

function resetButtons(pathStillSnapshot, pathAnimatedSnapshot, notify) {
    // If we're not taking an animated snapshot, we have to show the HUD.
    // If we ARE taking an animated snapshot, we've already re-enabled the HUD by this point.
    if (pathAnimatedSnapshot === "") {
        // show hud

        Reticle.visible = reticleVisible;
        // show overlays if they were on
        if (resetOverlays) {
            Menu.setIsOptionChecked("Overlays", true);
        }
    } else {
        // Allow the user to click the snapshot HUD button again
        if (!buttonConnected) {
            button.clicked.connect(onClicked);
            buttonConnected = true;
        }
    }
    Window.snapshotTaken.disconnect(resetButtons);

    // A Snapshot Review dialog might be left open indefinitely after taking the picture,
    // during which time the user may have moved. So stash that info in the dialog so that
    // it records the correct href. (We can also stash in .jpegs, but not .gifs.)
    // last element in data array tells dialog whether we can share or not
    var confirmShareContents = [
        { localPath: pathStillSnapshot, href: href },
        {
            canShare: !!isDomainOpen(domainId),
            openFeedAfterShare: shouldOpenFeedAfterShare()
        }];
    if (pathAnimatedSnapshot !== "") {
        confirmShareContents.unshift({ localPath: pathAnimatedSnapshot, href: href });
    }
    confirmShare(confirmShareContents);
    if (clearOverlayWhenMoving) {
        MyAvatar.setClearOverlayWhenMoving(true); // not until after the share dialog
    }
    HMD.openTablet();
}

function processingGif() {
    // show hud
    Reticle.visible = reticleVisible;
    button.clicked.disconnect(onClicked);
    buttonConnected = false;
    // show overlays if they were on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", true);
    }
}

function onTabletScreenChanged(type, url) {
    if (isInSnapshotReview) {
        tablet.webEventReceived.disconnect(onMessage);
        isInSnapshotReview = false;
    }
}
function onConnected() {
    if (shareAfterLogin && Account.isLoggedIn()) {
        print('sharing', snapshotToShareAfterLogin.path);
        Window.shareSnapshot(snapshotToShareAfterLogin.path, snapshotToShareAfterLogin.href);
        shareAfterLogin = false;
        if (shouldOpenFeedAfterShare()) {
            showFeedWindow();
        }
    }
    
}

button.clicked.connect(onClicked);
buttonConnected = true;
Window.snapshotShared.connect(snapshotShared);
Window.processingGif.connect(processingGif);
tablet.screenChanged.connect(onTabletScreenChanged);
Account.usernameChanged.connect(onConnected);
Script.scriptEnding.connect(function () {
    if (buttonConnected) {
        button.clicked.disconnect(onClicked);
        buttonConnected = false;
    }
    if (tablet) {
        tablet.removeButton(button);
    }
    Window.snapshotShared.disconnect(snapshotShared);
    Window.processingGif.disconnect(processingGif);
    tablet.screenChanged.disconnect(onTabletScreenChanged);
});

}()); // END LOCAL_SCOPE
