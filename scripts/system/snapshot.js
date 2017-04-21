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

var snapshotOptions;
var imageData;
var shareAfterLogin = false;
var snapshotToShareAfterLogin;
var METAVERSE_BASE = location.metaverseServerUrl;

function request(options, callback) { // cb(error, responseOfCorrectContentType) of url. A subset of npm request.
    var httpRequest = new XMLHttpRequest(), key;
    // QT bug: apparently doesn't handle onload. Workaround using readyState.
    httpRequest.onreadystatechange = function () {
        var READY_STATE_DONE = 4;
        var HTTP_OK = 200;
        if (httpRequest.readyState >= READY_STATE_DONE) {
            var error = (httpRequest.status !== HTTP_OK) && httpRequest.status.toString() + ':' + httpRequest.statusText,
                response = !error && httpRequest.responseText,
                contentType = !error && httpRequest.getResponseHeader('content-type');
            if (!error && contentType.indexOf('application/json') === 0) { // ignoring charset, etc.
                try {
                    response = JSON.parse(response);
                } catch (e) {
                    error = e;
                }
            }
            callback(error, response);
        }
    };
    if (typeof options === 'string') {
        options = { uri: options };
    }
    if (options.url) {
        options.uri = options.url;
    }
    if (!options.method) {
        options.method = 'GET';
    }
    if (options.body && (options.method === 'GET')) { // add query parameters
        var params = [], appender = (-1 === options.uri.search('?')) ? '?' : '&';
        for (key in options.body) {
            params.push(key + '=' + options.body[key]);
        }
        options.uri += appender + params.join('&');
        delete options.body;
    }
    if (options.json) {
        options.headers = options.headers || {};
        options.headers["Content-type"] = "application/json";
        options.body = JSON.stringify(options.body);
    }
    for (key in options.headers || {}) {
        httpRequest.setRequestHeader(key, options.headers[key]);
    }
    httpRequest.open(options.method, options.uri, true);
    httpRequest.send(options.body);
}

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
                action: "captureSettings",
                setting: Settings.getValue("alsoTakeAnimatedSnapshot", true)
            }));
            tablet.emitScriptEvent(JSON.stringify({
                type: "snapshot",
                action: "addImages",
                options: snapshotOptions,
                data: imageData
            }));
            break;
        case 'openSettings':
            if ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
                    || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar"))) {
                Desktop.show("hifi/dialogs/GeneralPreferencesDialog.qml", "General Preferences");
            } else {
                tablet.loadQMLOnTop("TabletGeneralPreferences.qml");
            }
            break;
        case 'captureStillAndGif':
            print("Changing Snapshot Capture Settings to Capture Still + GIF");
            Settings.setValue("alsoTakeAnimatedSnapshot", true);
            break;
        case 'captureStillOnly':
            print("Changing Snapshot Capture Settings to Capture Still Only");
            Settings.setValue("alsoTakeAnimatedSnapshot", false);
            break;
        case 'takeSnapshot':
            // In settings, first store the paths to the last snapshot
            //
            onClicked();
            break;
        case 'shareSnapshotForUrl':
            isLoggedIn = Account.isLoggedIn();
            if (isLoggedIn) {
                print('Sharing snapshot with audience "for_url":', message.data);
                Window.shareSnapshot(message.data, message.href || href);
            } else {
                // TODO?
            }
            break;
        case 'shareSnapshotWithEveryone':
            isLoggedIn = Account.isLoggedIn();
            if (isLoggedIn) {
                print('sharing', message.data);
                request({
                    uri: METAVERSE_BASE + '/api/v1/user_stories/' + story_id,
                    method: 'PUT',
                    json: true,
                    body: {
                        audience: "for_feed",
                    }
                }, function (error, response) {
                    if (error || (response.status !== 'success')) {
                        print("Error changing audience: ", error || response.status);
                        return;
                    }
                });
            } else {
                // TODO
                /*
                needsLogin = true;
                shareAfterLogin = true;
                snapshotToShareAfterLogin = { path: message.data, href: message.href || href };
                */
            }
            /*
            if (needsLogin) {
                if ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
                    || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar"))) {
                    Menu.triggerOption("Login / Sign Up");
                } else {
                    tablet.loadQMLOnTop("../../dialogs/TabletLoginDialog.qml");
                    HMD.openTablet();
                }
            }*/
            break;
        default:
            print('Unknown message action received by snapshot.js!');
            break;
    }
}

var SNAPSHOT_REVIEW_URL = Script.resolvePath("html/SnapshotReview.html");
var isInSnapshotReview = false;
function reviewSnapshot() {
    tablet.gotoWebScreen(SNAPSHOT_REVIEW_URL);
    tablet.webEventReceived.connect(onMessage);
    HMD.openTablet();
    isInSnapshotReview = true;
}

function snapshotUploaded(isError, reply) {
    if (!isError) {
        print('SUCCESS: Snapshot uploaded! Story with audience:for_url created! ID:', reply);
        tablet.emitScriptEvent(JSON.stringify({
            type: "snapshot",
            action: "enableShareButtons"
        }));
    } else {
        print(reply);
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
    Window.stillSnapshotTaken.connect(stillSnapshotTaken);
    Window.processingGifStarted.connect(processingGifStarted);
    Window.processingGifCompleted.connect(processingGifCompleted);

    // hide overlays if they are on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", false);
    }

    // take snapshot (with no notification)
    Script.setTimeout(function () {
        HMD.closeTablet();
        Script.setTimeout(function () {
            Window.takeSnapshot(false, Settings.getValue("alsoTakeAnimatedSnapshot", true), 1.91);
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

function stillSnapshotTaken(pathStillSnapshot, notify) {
    // show hud
    Reticle.visible = reticleVisible;
    // show overlays if they were on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", true);
    }
    Window.stillSnapshotTaken.disconnect(stillSnapshotTaken);

    // A Snapshot Review dialog might be left open indefinitely after taking the picture,
    // during which time the user may have moved. So stash that info in the dialog so that
    // it records the correct href. (We can also stash in .jpegs, but not .gifs.)
    // last element in data array tells dialog whether we can share or not
    snapshotOptions = {
        containsGif: false,
        processingGif: false,
        canShare: !!isDomainOpen(domainId)
    };
    imageData = [{ localPath: pathStillSnapshot, href: href }];
    reviewSnapshot();
    if (clearOverlayWhenMoving) {
        MyAvatar.setClearOverlayWhenMoving(true); // not until after the share dialog
    }
    HMD.openTablet();
}

function processingGifStarted(pathStillSnapshot) {
    Window.processingGifStarted.disconnect(processingGifStarted);
    if (buttonConnected) {
        button.clicked.disconnect(onClicked);
        buttonConnected = false;
    }
    // show hud
    Reticle.visible = reticleVisible;
    // show overlays if they were on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", true);
    }

    snapshotOptions = {
        containsGif: true,
        processingGif: true,
        loadingGifPath: Script.resolvePath(Script.resourcesPath() + 'icons/loadingDark.gif'),
        canShare: !!isDomainOpen(domainId)
    };
    imageData = [{ localPath: pathStillSnapshot, href: href }];
    reviewSnapshot();
    if (clearOverlayWhenMoving) {
        MyAvatar.setClearOverlayWhenMoving(true); // not until after the share dialog
    }
    HMD.openTablet();
}

function processingGifCompleted(pathAnimatedSnapshot) {
    Window.processingGifCompleted.disconnect(processingGifCompleted);
    if (!buttonConnected) {
        button.clicked.connect(onClicked);
        buttonConnected = true;
    }

    snapshotOptions = {
        containsGif: true,
        processingGif: false,
        canShare: !!isDomainOpen(domainId)
    }
    imageData = [{ localPath: pathAnimatedSnapshot, href: href }];

    tablet.emitScriptEvent(JSON.stringify({
        type: "snapshot",
        action: "addImages",
        options: snapshotOptions,
        data: imageData
    }));
}

function onTabletScreenChanged(type, url) {
    if (isInSnapshotReview) {
        tablet.webEventReceived.disconnect(onMessage);
        isInSnapshotReview = false;
    }
}
function onConnected() {
    if (shareAfterLogin && Account.isLoggedIn()) {
        print('Sharing snapshot after login:', snapshotToShareAfterLogin.path);
        Window.shareSnapshot(snapshotToShareAfterLogin.path, snapshotToShareAfterLogin.href);
        shareAfterLogin = false;
    }
}

button.clicked.connect(onClicked);
buttonConnected = true;
Window.snapshotShared.connect(snapshotUploaded);
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
    Window.snapshotShared.disconnect(snapshotUploaded);
    tablet.screenChanged.disconnect(onTabletScreenChanged);
});

}()); // END LOCAL_SCOPE
