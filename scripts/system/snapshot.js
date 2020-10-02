//
// snapshot.js
//
// Created by David Kelly on 1 August 2016
// Copyright 2016 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Tablet, Script, HMD, Settings, DialogsManager, Menu, Reticle,
    OverlayWebWindow, Desktop, Account, MyAvatar, Snapshot */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function () { // BEGIN LOCAL_SCOPE
Script.include("/~/system/libraries/accountUtils.js");
var AppUi = Script.require('appUi');

var SNAPSHOT_DELAY = 500; // 500ms
var FINISH_SOUND_DELAY = 350;
var resetOverlays;
var reticleVisible;

var snapshotOptions = {};
var imageData = [];
var storyIDsToMaybeDelete = [];
var shareAfterLogin = false;
var snapshotToShareAfterLogin = [];
var METAVERSE_BASE = Account.metaverseServerURL;
var isLoggedIn;
var mostRecentGifSnapshotFilename = "";
var mostRecentStillSnapshotFilename = "";

// It's totally unnecessary to return to C++ to perform many of these requests, such as DELETEing an old story,
// POSTING a new one, PUTTING a new audience, or GETTING story data. It's far more efficient to do all of that within JS
var request;

try {
    // Due to an issue where if the user spams 'script reload', this call could cause an exception
    // preventing our scriptEnding to not properly be initialized resulting in the tablet button
    // duplicating itself where you end up with a bunch of SNAP buttons on your toolbar
    request = Script.require('request').request;
} catch(err) {
    print('Failed to resolve request api, error: ' + err);
}

function removeFromStoryIDsToMaybeDelete(story_id) {
    story_id = parseInt(story_id);
    if (storyIDsToMaybeDelete.indexOf(story_id) > -1) {
        storyIDsToMaybeDelete.splice(storyIDsToMaybeDelete.indexOf(story_id), 1);
    }
    print('storyIDsToMaybeDelete[] now:', JSON.stringify(storyIDsToMaybeDelete));
}

function fileExtensionMatches(filePath, extension) {
    return filePath.split('.').pop().toLowerCase() === extension;
}

function getFilenameFromPath(str) {
    return str.split('\\').pop().split('/').pop();
}

function onMessage(message) {
    // Receives message from the html dialog via the qwebchannel EventBridge. This is complicated by the following:
    // 1. Although we can send POJOs, we cannot receive a toplevel object. (Arrays of POJOs are fine, though.)
    // 2. Although we currently use a single image, we would like to take snapshot, a selfie, a 360 etc. all at the
    //    same time, show the user all of them, and have the user deselect any that they do not want to share.
    //    So we'll ultimately be receiving a set of objects, perhaps with different post processing for each.

    if (message.type !== "snapshot") {
        return;
    }

    switch (message.action) {
        case 'ready': // DOM is ready and page has loaded
            ui.sendMessage({
                type: "snapshot",
                action: "captureSettings",
                setting: Settings.getValue("alsoTakeAnimatedSnapshot", true)
            });
            if (Snapshot.getSnapshotsLocation() !== "") {
                isDomainOpen(Settings.getValue("previousSnapshotDomainID"), function (canShare) {
                    ui.sendMessage({
                        type: "snapshot",
                        action: "showPreviousImages",
                        options: snapshotOptions,
                        image_data: imageData,
                        canShare: canShare
                    });
                });
            } else {
                ui.sendMessage({
                    type: "snapshot",
                    action: "showSetupInstructions"
                });
                Settings.setValue("previousStillSnapPath", "");
                Settings.setValue("previousStillSnapStoryID", "");
                Settings.setValue("previousStillSnapBlastingDisabled", false);
                Settings.setValue("previousStillSnapHifiSharingDisabled", false);
                Settings.setValue("previousStillSnapUrl", "");
                Settings.setValue("previousAnimatedSnapPath", "");
                Settings.setValue("previousAnimatedSnapStoryID", "");
                Settings.setValue("previousAnimatedSnapBlastingDisabled", false);
                Settings.setValue("previousAnimatedSnapHifiSharingDisabled", false);
            }
            updatePrintPermissions();
            break;
        case 'login':
            openLoginWindow();
            break;
        case 'chooseSnapshotLocation':
            Window.browseDirChanged.connect(snapshotDirChanged);
            Window.browseDirAsync("Choose Snapshots Directory", "", "");
            break;
        case 'openSettings':
            if ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar", false))
                || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar", true))) {
                Desktop.show("hifi/dialogs/GeneralPreferencesDialog.qml", "GeneralPreferencesDialog");
            } else {
                ui.openNewAppOnTop("hifi/tablet/TabletGeneralPreferences.qml");
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
            takeSnapshot();
            break;
        case 'shareSnapshotForUrl':
            isDomainOpen(Settings.getValue("previousSnapshotDomainID"), function (canShare) {
                var isGif = fileExtensionMatches(message.data, "gif");      
                isLoggedIn = Account.isLoggedIn();                        
                if (!isGif) {
                    isUploadingPrintableStill = canShare && Account.isLoggedIn();
                }
                if (canShare) {                  
                    if (isLoggedIn) {
                        print('Sharing snapshot with audience "for_url":', message.data);
                        Window.shareSnapshot(message.data, Settings.getValue("previousSnapshotHref"));                      
                        if (isGif) {
                            mostRecentGifSnapshotFilename = getFilenameFromPath(message.data);
                        } else {
                            mostRecentStillSnapshotFilename = getFilenameFromPath(message.data);
                        }
                    } else {
                        shareAfterLogin = true;
                        snapshotToShareAfterLogin.push({ path: message.data, href: Settings.getValue("previousSnapshotHref") });
                    }
                }
                updatePrintPermissions();
            });
            break;
        case 'blastToConnections':
            isLoggedIn = Account.isLoggedIn();
            if (isLoggedIn) {
                if (message.isGif) {
                    Settings.setValue("previousAnimatedSnapBlastingDisabled", true);
                } else {
                    Settings.setValue("previousStillSnapBlastingDisabled", true);
                }

                print('Uploading new story for announcement!');

                request({
                    uri: METAVERSE_BASE + '/api/v1/user_stories/' + message.story_id,
                    method: 'GET'
                }, function (error, response) {
                    if (error || (response.status !== 'success')) {
                        print("ERROR getting details about existing snapshot story:", error || response.status);
                        return;
                    } else {
                        var requestBody = {
                            user_story: {
                                audience: "for_connections",
                                action: "announcement",
                                path: response.user_story.path,
                                place_name: response.user_story.place_name,
                                thumbnail_url: response.user_story.thumbnail_url,
                                // For historical reasons, the server doesn't take nested JSON objects.
                                // Thus, I'm required to STRINGIFY what should be a nested object.
                                details: JSON.stringify({
                                    shareable_url: response.user_story.details.shareable_url,
                                    image_url: response.user_story.details.image_url
                                })
                            }
                        }
                        request({
                            uri: METAVERSE_BASE + '/api/v1/user_stories',
                            method: 'POST',
                            json: true,
                            body: requestBody
                        }, function (error, response) {
                            if (error || (response.status !== 'success')) {
                                print("ERROR uploading announcement story: ", error || response.status);
                                if (message.isGif) {
                                    Settings.setValue("previousAnimatedSnapBlastingDisabled", false);
                                } else {
                                    Settings.setValue("previousStillSnapBlastingDisabled", false);
                                }
                                return;
                            } else {
                                print("SUCCESS uploading announcement story! Story ID:", response.user_story.id);
                                removeFromStoryIDsToMaybeDelete(message.story_id); // Don't delete original "for_url" story
                            }
                        });
                    }
                });
            }
            break;
        case 'requestPrintButtonUpdate':
            updatePrintPermissions();
            break;
        case 'printToPolaroid':
            if (Entities.canRez() || Entities.canRezTmp()) {
               printToPolaroid(Settings.getValue("previousStillSnapUrl"));
               removeFromStoryIDsToMaybeDelete(Settings.getValue("previousStillSnapStoryID"));
            }
            break;
        case 'alertSnapshotLoadFailed':
            snapshotFailedToLoad = true;
            break;
        case 'shareSnapshotWithEveryone':
            isLoggedIn = Account.isLoggedIn();
            if (isLoggedIn) {
                if (message.isGif) {
                    Settings.setValue("previousAnimatedSnapHifiSharingDisabled", true);
                } else {
                    Settings.setValue("previousStillSnapHifiSharingDisabled", true);
                }
                print('Modifying audience of story ID', message.story_id, "to 'for_feed'");
                var requestBody = {
                    audience: "for_feed"
                }

                if (message.isAnnouncement) {
                    requestBody.action = "announcement";
                    print('...Also announcing!');
                }
                request({
                    uri: METAVERSE_BASE + '/api/v1/user_stories/' + message.story_id,
                    method: 'PUT',
                    json: true,
                    body: requestBody
                }, function (error, response) {
                    if (error || (response.status !== 'success')) {
                        print("ERROR changing audience: ", error || response.status);
                        if (message.isGif) {
                            Settings.setValue("previousAnimatedSnapHifiSharingDisabled", false);
                        } else {
                            Settings.setValue("previousStillSnapHifiSharingDisabled", false);
                        }
                        return;
                    } else {
                        print("SUCCESS changing audience" + (message.isAnnouncement ? " and posting announcement!" : "!"));
                        removeFromStoryIDsToMaybeDelete(message.story_id);
                    }
                });
            }
            break;
        case 'removeFromStoryIDsToMaybeDelete':
            removeFromStoryIDsToMaybeDelete(message.story_id);
            break;
        default:
            print('Unknown message action received by snapshot.js!');
            break;
    }
}

var POLAROID_PRINT_SOUND = SoundCache.getSound(Script.resourcesPath() + "sounds/snapshot/sound-print-photo.wav");
var POLAROID_MODEL_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/alan/dev/Test/snapshot.fbx");
var POLAROID_RATE_LIMIT_MS = 1000;
var polaroidPrintingIsRateLimited = false;

// force call the gotoPreviousApp on script thead to load snapshot html page.
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
tablet.fromQml.connect(function(message) {
    if (message === 'returnToPreviousApp') {
	tablet.returnToPreviousApp();
    }
});

function printToPolaroid(image_url) {
    // Rate-limit printing
    if (polaroidPrintingIsRateLimited) {
        return;
    }
    polaroidPrintingIsRateLimited = true;
    Script.setTimeout(function () {
        polaroidPrintingIsRateLimited = false;
    }, POLAROID_RATE_LIMIT_MS);

    var polaroid_url = image_url;                  

    var model_pos = Vec3.sum(MyAvatar.position, Vec3.multiply(1.25, Quat.getForward(MyAvatar.orientation)));
    model_pos.y += 0.39; // Print a bit closer to the head
    
    var model_q1 = MyAvatar.orientation;
    var model_q2 = Quat.angleAxis(90, Quat.getRight(model_q1));
    var model_rot = Quat.multiply(model_q2, model_q1);
    
    var properties = {                                
        "type": 'Model',
        "shapeType": 'box',

        "name": "Snapshot by " + MyAvatar.sessionDisplayName,
        "description": "Printed from SNAP app",                               
        "modelURL": POLAROID_MODEL_URL,

        "dimensions": { "x": 0.5667, "y": 0.042, "z": 0.4176 },
        "position": model_pos,
        "rotation": model_rot,

        "textures": JSON.stringify( { "tex.picture": polaroid_url } ),

        "density": 200,
        "restitution": 0.15,                            
        "gravity": { "x": 0, "y": -2.0, "z": 0 },
        "damping": 0.45,

        "dynamic": true, 
        "collisionsWillMove": true,

        "grab": { "grabbable": true }
    };
    
    var polaroid = Entities.addEntity(properties);
    Audio.playSound(POLAROID_PRINT_SOUND, {
        position: model_pos,
        localOnly: false,
        volume: 0.2
    });
}

function fillImageDataFromPrevious() {
    isLoggedIn = Account.isLoggedIn();
    var previousStillSnapPath = Settings.getValue("previousStillSnapPath");
    var previousStillSnapStoryID = Settings.getValue("previousStillSnapStoryID");
    var previousStillSnapBlastingDisabled = Settings.getValue("previousStillSnapBlastingDisabled");
    var previousStillSnapHifiSharingDisabled = Settings.getValue("previousStillSnapHifiSharingDisabled");
    var previousAnimatedSnapPath = Settings.getValue("previousAnimatedSnapPath");
    var previousAnimatedSnapStoryID = Settings.getValue("previousAnimatedSnapStoryID");
    var previousAnimatedSnapBlastingDisabled = Settings.getValue("previousAnimatedSnapBlastingDisabled");
    var previousAnimatedSnapHifiSharingDisabled = Settings.getValue("previousAnimatedSnapHifiSharingDisabled");

    snapshotOptions = {
        containsGif: previousAnimatedSnapPath !== "",
        processingGif: false,
        shouldUpload: false,
        canBlast: snapshotDomainID === Settings.getValue("previousSnapshotDomainID") &&
            snapshotDomainID === location.domainID,
        isLoggedIn: isLoggedIn
    };
    imageData = [];
    if (previousStillSnapPath !== "") {
        imageData.push({
            localPath: previousStillSnapPath,
            story_id: previousStillSnapStoryID,
            blastButtonDisabled: previousStillSnapBlastingDisabled,
            hifiButtonDisabled: previousStillSnapHifiSharingDisabled,
            errorPath: Script.resourcesPath() + 'snapshot/img/no-image.jpg'
        });
    }
    if (previousAnimatedSnapPath !== "") {
        imageData.push({
            localPath: previousAnimatedSnapPath,
            story_id: previousAnimatedSnapStoryID,
            blastButtonDisabled: previousAnimatedSnapBlastingDisabled,
            hifiButtonDisabled: previousAnimatedSnapHifiSharingDisabled,
            errorPath: Script.resourcesPath() + 'snapshot/img/no-image.jpg'
        });
    }
}

function snapshotUploaded(isError, reply) {
    if (!isError) {
        var replyJson = JSON.parse(reply),
            storyID = replyJson.user_story.id,
            imageURL = replyJson.user_story.details.image_url,
            isGif = fileExtensionMatches(imageURL, "gif"),
            ignoreGifSnapshotData = false,
            ignoreStillSnapshotData = false;
        storyIDsToMaybeDelete.push(parseInt(storyID));
        print('storyIDsToMaybeDelete[] now:', JSON.stringify(storyIDsToMaybeDelete));
        if (isGif) {
            if (mostRecentGifSnapshotFilename !== replyJson.user_story.details.original_image_file_name) {
                ignoreGifSnapshotData = true;
            }
        } else {
            if (mostRecentStillSnapshotFilename !== replyJson.user_story.details.original_image_file_name) {
                ignoreStillSnapshotData = true;
            }
        }
        if ((isGif && !ignoreGifSnapshotData) || (!isGif && !ignoreStillSnapshotData)) {
            print('SUCCESS: Snapshot uploaded! Story with audience:for_url created! ID:', storyID);
            ui.sendMessage({
                type: "snapshot",
                action: "snapshotUploadComplete",
                story_id: storyID,
                image_url: imageURL,
            });
            if (isGif) {
                Settings.setValue("previousAnimatedSnapStoryID", storyID);
            } else {
                Settings.setValue("previousStillSnapStoryID", storyID);
                Settings.setValue("previousStillSnapUrl", imageURL);
            }
        } else {
            print('Ignoring snapshotUploaded() callback for stale ' + (isGif ? 'GIF' : 'Still' ) + ' snapshot. Stale story ID:', storyID);
        }
    }
    isUploadingPrintableStill = false;
}
var href, snapshotDomainID;
function takeSnapshot() {
    ui.sendMessage({
        type: "snapshot",
        action: "clearPreviousImages"
    });
    Settings.setValue("previousStillSnapPath", "");
    Settings.setValue("previousStillSnapStoryID", "");
    Settings.setValue("previousStillSnapBlastingDisabled", false);
    Settings.setValue("previousStillSnapHifiSharingDisabled", false);
    Settings.setValue("previousStillSnapUrl", "");
    Settings.setValue("previousAnimatedSnapPath", "");
    Settings.setValue("previousAnimatedSnapStoryID", "");
    Settings.setValue("previousAnimatedSnapBlastingDisabled", false);
    Settings.setValue("previousAnimatedSnapHifiSharingDisabled", false);
    
    // Since we are taking a snapshot, we should make the print button appear to be loading/processing
    snapshotFailedToLoad = false;
    isUploadingPrintableStill = true;
    updatePrintPermissions();

    // We will record snapshots based on the starting location. That could change, e.g., when recording a .gif.
    // Even the domainID could change (e.g., if the user falls into a teleporter while recording).
    href = location.href;
    Settings.setValue("previousSnapshotHref", href);
    snapshotDomainID = location.domainID;
    Settings.setValue("previousSnapshotDomainID", snapshotDomainID);

    maybeDeleteSnapshotStories();

    // update button states
    resetOverlays = Menu.isOptionChecked("Show Overlays"); // For completeness. Certainly true if the button is visible to be clicked.
    reticleVisible = Reticle.visible;
    Reticle.visible = false;
    if (!HMD.active) {
        Reticle.allowMouseCapture = false;
    }
    
    var includeAnimated = Settings.getValue("alsoTakeAnimatedSnapshot", true);
    if (includeAnimated) {
        Window.processingGifStarted.connect(processingGifStarted);
    } else {
        Window.stillSnapshotTaken.connect(stillSnapshotTaken);
    }

    // hide overlays if they are on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Show Overlays", false);
    }

    var snapActivateSound = SoundCache.getSound(Script.resourcesPath() + "sounds/snapshot/snap.wav");

    // take snapshot (with no notification)
    Script.setTimeout(function () {
        Audio.playSound(snapActivateSound, {
            position: { x: MyAvatar.position.x, y: MyAvatar.position.y, z: MyAvatar.position.z },
            localOnly: true,
            volume: 1.0
        });
        HMD.closeTablet();
        var DOUBLE_RENDER_TIME_TO_MS = 2000; // If rendering is bogged down, allow double the render time to close the tablet.
        Script.setTimeout(function () {
            Window.takeSnapshot(false, includeAnimated, 1.91);
        }, Math.max(SNAPSHOT_DELAY, DOUBLE_RENDER_TIME_TO_MS / Rates.render ));
    }, FINISH_SOUND_DELAY);
    UserActivityLogger.logAction("snaphshot_taken", { location: location.href });
}
    
function isDomainOpen(id, callback) {
    print("Checking open status of domain with ID:", id);
    var status = false;
    if (id) {
        var options = [
            'now=' + new Date().toISOString(),
            'include_actions=concurrency',
            'domain_id=' + id.slice(1, -1),
            'restriction=open,hifi' // If we're sharing, we're logged in
            // If we're here, protocol matches, and it is online
        ];
        var url = METAVERSE_BASE + "/api/v1/user_stories?" + options.join('&');

        request({
            uri: url,
            method: 'GET'
        }, function (error, response) {
            if (error || (response.status !== 'success')) {
                print("ERROR getting open status of domain: ", error || response.status);
            } else {
                status = response.total_entries ? true : false;
            }
            print("Domain open status:", status);
            callback(status);
        });
    } else {
        callback(status);
    }
}

function stillSnapshotTaken(pathStillSnapshot, notify) {
    isLoggedIn = Account.isLoggedIn();
    // show hud
    Reticle.visible = reticleVisible;
    Reticle.allowMouseCapture = true;
    // show overlays if they were on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Show Overlays", true);
    }
    Window.stillSnapshotTaken.disconnect(stillSnapshotTaken);

    // A Snapshot Review dialog might be left open indefinitely after taking the picture,
    // during which time the user may have moved. So stash that info in the dialog so that
    // it records the correct href. (We can also stash in .jpegs, but not .gifs.)
    // last element in data array tells dialog whether we can share or not
    Settings.setValue("previousStillSnapPath", pathStillSnapshot);

    HMD.openTablet();

    isDomainOpen(snapshotDomainID, function (canShare) {
        snapshotOptions = {
            containsGif: false,
            processingGif: false,
            canShare: canShare,
            isLoggedIn: isLoggedIn
        };
        imageData = [{ localPath: pathStillSnapshot, href: href }];
        ui.sendMessage({
            type: "snapshot",
            action: "addImages",
            options: snapshotOptions,
            image_data: imageData
        });
    });
}

function snapshotDirChanged(snapshotPath) {
    Window.browseDirChanged.disconnect(snapshotDirChanged);
    if (snapshotPath !== "") { // not cancelled
        Snapshot.setSnapshotsLocation(snapshotPath);
        ui.sendMessage({
            type: "snapshot",
            action: "snapshotLocationChosen"
        });
    }
}

function processingGifStarted(pathStillSnapshot) {
    Window.processingGifStarted.disconnect(processingGifStarted);
    Window.processingGifCompleted.connect(processingGifCompleted);
    isLoggedIn = Account.isLoggedIn();
    // show hud
    Reticle.visible = reticleVisible;
    Reticle.allowMouseCapture = true;
    // show overlays if they were on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Show Overlays", true);
    }
    Settings.setValue("previousStillSnapPath", pathStillSnapshot);

    HMD.openTablet();
    
    isDomainOpen(snapshotDomainID, function (canShare) {
        snapshotOptions = {
            containsGif: true,
            processingGif: true,
            loadingGifPath: Script.resourcesPath() + 'icons/loadingDark.gif',
            canShare: canShare,
            isLoggedIn: isLoggedIn
        };
        imageData = [{ localPath: pathStillSnapshot, href: href }];
        ui.sendMessage({
            type: "snapshot",
            action: "addImages",
            options: snapshotOptions,
            image_data: imageData
        });
    });
}

function processingGifCompleted(pathAnimatedSnapshot) {
    isLoggedIn = Account.isLoggedIn();
    Window.processingGifCompleted.disconnect(processingGifCompleted);

    Settings.setValue("previousAnimatedSnapPath", pathAnimatedSnapshot);

    isDomainOpen(snapshotDomainID, function (canShare) {
        snapshotOptions = {
            containsGif: true,
            processingGif: false,
            canShare: canShare,
            isLoggedIn: isLoggedIn,
            canBlast: location.domainID === Settings.getValue("previousSnapshotDomainID"),
        };
        imageData = [{ localPath: pathAnimatedSnapshot, href: href }];
        ui.sendMessage({
            type: "snapshot",
            action: "addImages",
            options: snapshotOptions,
            image_data: imageData
        });
    });
}
function maybeDeleteSnapshotStories() {
    storyIDsToMaybeDelete.forEach(function (element, idx, array) {
        request({
            uri: METAVERSE_BASE + '/api/v1/user_stories/' + element,
            method: 'DELETE'
        }, function (error, response) {
            if (error || (response.status !== 'success')) {
                print("ERROR deleting snapshot story: ", error || response.status);
                return;
            } else {
                print("SUCCESS deleting snapshot story with ID", element);
            }
        })
    });
    storyIDsToMaybeDelete = [];
}
function onUsernameChanged() {
    fillImageDataFromPrevious();
    isDomainOpen(Settings.getValue("previousSnapshotDomainID"), function (canShare) {
        ui.sendMessage({
            type: "snapshot",
            action: "showPreviousImages",
            options: snapshotOptions,
            image_data: imageData,
            canShare: canShare
        });
    });
    if (isLoggedIn) {
        if (shareAfterLogin) {
            isDomainOpen(Settings.getValue("previousSnapshotDomainID"), function (canShare) {
                if (canShare) {
                    snapshotToShareAfterLogin.forEach(function (element) {
                        print('Uploading snapshot after login:', element.path);
                        Window.shareSnapshot(element.path, element.href);
                        var isGif = fileExtensionMatches(element.path, "gif");
                        if (isGif) {
                            mostRecentGifSnapshotFilename = getFilenameFromPath(element.path);
                        } else {
                            mostRecentStillSnapshotFilename = getFilenameFromPath(element.path);
                        }
                    });
                }
                isUploadingPrintableStill = canShare;
                updatePrintPermissions();
            });

            shareAfterLogin = false;
            snapshotToShareAfterLogin = [];
        }
    }
}

function snapshotLocationSet(location) {
    if (location !== "") {
        ui.sendMessage({
            type: "snapshot",
            action: "snapshotLocationChosen"
        });
    }
}

function updatePrintPermissions() {
    processRezPermissionChange(Entities.canRez() || Entities.canRezTmp());
}

var snapshotFailedToLoad = false;
var isUploadingPrintableStill = false;
function processRezPermissionChange(canRez) {
    var action = "";
    
    if (canRez && !snapshotFailedToLoad) {  
        if (Settings.getValue("previousStillSnapUrl")) {
            action = 'setPrintButtonEnabled';
        } else if (isUploadingPrintableStill) {
            action = 'setPrintButtonLoading';
        } else {
            action = 'setPrintButtonDisabled';
        }
    } else {
        action = 'setPrintButtonDisabled';
    }
    
    ui.sendMessage({
        type: "snapshot",
        action : action
    });
}

function startup() {
    ui = new AppUi({
        buttonName: "SNAP",
        sortOrder: 5,
        home: Script.resolvePath("html/SnapshotReview.html"),
        onOpened: fillImageDataFromPrevious,
        onMessage: onMessage
    });

    Entities.canRezChanged.connect(updatePrintPermissions);
    Entities.canRezTmpChanged.connect(updatePrintPermissions);
    GlobalServices.myUsernameChanged.connect(onUsernameChanged);
    Snapshot.snapshotLocationSet.connect(snapshotLocationSet);
    Window.snapshotShared.connect(snapshotUploaded);
}
startup();

function shutdown() {
    Window.snapshotShared.disconnect(snapshotUploaded);
    Snapshot.snapshotLocationSet.disconnect(snapshotLocationSet);
    GlobalServices.myUsernameChanged.disconnect(onUsernameChanged);
    Entities.canRezChanged.disconnect(updatePrintPermissions);
    Entities.canRezTmpChanged.disconnect(updatePrintPermissions);
}
Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
