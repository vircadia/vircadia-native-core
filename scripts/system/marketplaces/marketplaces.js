//
//  marketplaces.js
//
//  Created by Eric Levin on 8 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Tablet, Script, HMD, UserActivityLogger, Entities, Account, Wallet, ContextOverlay, Settings, Camera, Vec3,
   Quat, MyAvatar, Clipboard, Menu, Grid, Uuid, GlobalServices, openLoginWindow, getConnectionData, Overlays, SoundCache,
   DesktopPreviewProvider */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

var selectionDisplay = null; // for gridTool.js to ignore

(function () { // BEGIN LOCAL_SCOPE
var AppUi = Script.require('appUi');
Script.include("/~/system/libraries/gridTool.js");
Script.include("/~/system/libraries/connectionUtils.js");

var METAVERSE_SERVER_URL = Account.metaverseServerURL;
var MARKETPLACES_URL = Script.resolvePath("../html/marketplaces.html");
var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");
var MARKETPLACE_CHECKOUT_QML_PATH = "hifi/commerce/checkout/Checkout.qml";
var MARKETPLACE_PURCHASES_QML_PATH = "hifi/commerce/purchases/Purchases.qml";
var MARKETPLACE_WALLET_QML_PATH = "hifi/commerce/wallet/Wallet.qml";
var MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH = "hifi/commerce/inspectionCertificate/InspectionCertificate.qml";
var REZZING_SOUND = SoundCache.getSound(Script.resolvePath("../assets/sounds/rezzing.wav"));

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

var CANCEL_BUTTON = 4194304; // QMessageBox::Cancel
var NO_BUTTON = 0; // QMessageBox::NoButton

var NO_PERMISSIONS_ERROR_MESSAGE = "Cannot download model because you can't write to \nthe domain's Asset Server.";

function onMessageBoxClosed(id, button) {
    if (id === messageBox && button === CANCEL_BUTTON) {
        isDownloadBeingCancelled = true;
        messageBox = null;
        ui.sendToHtml({
            type: CLARA_IO_CANCEL_DOWNLOAD
        });
    }
}

function onCanWriteAssetsChanged() {
    ui.sendToHtml({
        type: CAN_WRITE_ASSETS,
        canWriteAssets: Entities.canWriteAssets()
    });
}


var tabletShouldBeVisibleInSecondaryCamera = false;
function setTabletVisibleInSecondaryCamera(visibleInSecondaryCam) {
    if (visibleInSecondaryCam) {
        // if we're potentially showing the tablet, only do so if it was visible before
        if (!tabletShouldBeVisibleInSecondaryCamera) {
            return;
        }
    } else {
        // if we're hiding the tablet, check to see if it was visible in the first place
        tabletShouldBeVisibleInSecondaryCamera = Overlays.getProperty(HMD.tabletID, "isVisibleInSecondaryCamera");
    }

    Overlays.editOverlay(HMD.tabletID, { isVisibleInSecondaryCamera : visibleInSecondaryCam });
    Overlays.editOverlay(HMD.homeButtonID, { isVisibleInSecondaryCamera : visibleInSecondaryCam });
    Overlays.editOverlay(HMD.homeButtonHighlightID, { isVisibleInSecondaryCamera : visibleInSecondaryCam });
    Overlays.editOverlay(HMD.tabletScreenID, { isVisibleInSecondaryCamera : visibleInSecondaryCam });
}

function openWallet() {
    ui.open(MARKETPLACE_WALLET_QML_PATH);
}

// Function Name: wireQmlEventBridge()
//
// Description:
//   -Used to connect/disconnect the script's response to the tablet's "fromQml" signal. Set the "on" argument to enable or
//    disable to event bridge.
//
// Relevant Variables:
//   -hasEventBridge: true/false depending on whether we've already connected the event bridge.
var hasEventBridge = false;
function wireQmlEventBridge(on) {
    if (!ui.tablet) {
        print("Warning in wireQmlEventBridge(): 'tablet' undefined!");
        return;
    }
    if (on) {
        if (!hasEventBridge) {
            ui.tablet.fromQml.connect(onQmlMessageReceived);
            hasEventBridge = true;
        }
    } else {
        if (hasEventBridge) {
            ui.tablet.fromQml.disconnect(onQmlMessageReceived);
            hasEventBridge = false;
        }
    }
}

var contextOverlayEntity = "";
function openInspectionCertificateQML(currentEntityWithContextOverlay) {
    ui.open(MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH);
    contextOverlayEntity = currentEntityWithContextOverlay;
}

function setCertificateInfo(currentEntityWithContextOverlay, itemCertificateId) {
    var certificateId = itemCertificateId ||
        (Entities.getEntityProperties(currentEntityWithContextOverlay, ['certificateID']).certificateID);
    ui.tablet.sendToQml({
        method: 'inspectionCertificate_setCertificateId',
        entityId: currentEntityWithContextOverlay,
        certificateId: certificateId
    });
}

function onUsernameChanged() {
    if (onMarketplaceScreen) {
        ui.open(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
    }
}

var userHasUpdates = false;
function sendCommerceSettings() {
    ui.sendToHtml({
        type: "marketplaces",
        action: "commerceSetting",
        data: {
            commerceMode: Settings.getValue("commerce", true),
            userIsLoggedIn: Account.loggedIn,
            walletNeedsSetup: Wallet.walletStatus === 1,
            metaverseServerURL: Account.metaverseServerURL,
            messagesWaiting: userHasUpdates
        }
    });
}

// BEGIN AVATAR SELECTOR LOGIC
var UNSELECTED_COLOR = { red: 0x1F, green: 0xC6, blue: 0xA6 };
var SELECTED_COLOR = { red: 0xF3, green: 0x91, blue: 0x29 };
var HOVER_COLOR = { red: 0xD0, green: 0xD0, blue: 0xD0 };

var overlays = {}; // Keeps track of all our extended overlay data objects, keyed by target identifier.

function ExtendedOverlay(key, type, properties) { // A wrapper around overlays to store the key it is associated with.
    overlays[key] = this;
    this.key = key;
    this.selected = false;
    this.hovering = false;
    this.activeOverlay = Overlays.addOverlay(type, properties); // We could use different overlays for (un)selected...
}
// Instance methods:
ExtendedOverlay.prototype.deleteOverlay = function () { // remove display and data of this overlay
    Overlays.deleteOverlay(this.activeOverlay);
    delete overlays[this.key];
};

ExtendedOverlay.prototype.editOverlay = function (properties) { // change display of this overlay
    Overlays.editOverlay(this.activeOverlay, properties);
};

function color(selected, hovering) {
    var base = hovering ? HOVER_COLOR : selected ? SELECTED_COLOR : UNSELECTED_COLOR;
    function scale(component) {
        return component;
    }
    return { red: scale(base.red), green: scale(base.green), blue: scale(base.blue) };
}
// so we don't have to traverse the overlays to get the last one
var lastHoveringId = 0;
ExtendedOverlay.prototype.hover = function (hovering) {
    this.hovering = hovering;
    if (this.key === lastHoveringId) {
        if (hovering) {
            return;
        }
        lastHoveringId = 0;
    }
    this.editOverlay({ color: color(this.selected, hovering) });
    if (hovering) {
        // un-hover the last hovering overlay
        if (lastHoveringId && lastHoveringId !== this.key) {
            ExtendedOverlay.get(lastHoveringId).hover(false);
        }
        lastHoveringId = this.key;
    }
};
ExtendedOverlay.prototype.select = function (selected) {
    if (this.selected === selected) {
        return;
    }

    this.editOverlay({ color: color(selected, this.hovering) });
    this.selected = selected;
};
// Class methods:
var selectedId = false;
ExtendedOverlay.isSelected = function (id) {
    return selectedId === id;
};
ExtendedOverlay.get = function (key) { // answer the extended overlay data object associated with the given avatar identifier
    return overlays[key];
};
ExtendedOverlay.some = function (iterator) { // Bails early as soon as iterator returns truthy.
    var key;
    for (key in overlays) {
        if (iterator(ExtendedOverlay.get(key))) {
            return;
        }
    }
};
ExtendedOverlay.unHover = function () { // calls hover(false) on lastHoveringId (if any)
    if (lastHoveringId) {
        ExtendedOverlay.get(lastHoveringId).hover(false);
    }
};

// hit(overlay) on the one overlay intersected by pickRay, if any.
// noHit() if no ExtendedOverlay was intersected (helps with hover)
ExtendedOverlay.applyPickRay = function (pickRay, hit, noHit) {
    var pickedOverlay = Overlays.findRayIntersection(pickRay); // Depends on nearer coverOverlays to extend closer to us than farther ones.
    if (!pickedOverlay.intersects) {
        if (noHit) {
            return noHit();
        }
        return;
    }
    ExtendedOverlay.some(function (overlay) { // See if pickedOverlay is one of ours.
        if ((overlay.activeOverlay) === pickedOverlay.overlayID) {
            hit(overlay);
            return true;
        }
    });
};

function addAvatarNode(id) {
    return new ExtendedOverlay(id, "sphere", {
        drawInFront: true,
        solid: true,
        alpha: 0.8,
        color: color(false, false),
        ignoreRayIntersection: false
    });
}

var pingPong = true;
function updateOverlays() {
    var eye = Camera.position;
    AvatarList.getAvatarIdentifiers().forEach(function (id) {
        if (!id) {
            return; // don't update ourself, or avatars we're not interested in
        }
        var avatar = AvatarList.getAvatar(id);
        if (!avatar) {
            return; // will be deleted below if there had been an overlay.
        }
        var overlay = ExtendedOverlay.get(id);
        if (!overlay) { // For now, we're treating this as a temporary loss, as from the personal space bubble. Add it back.
            overlay = addAvatarNode(id);
        }
        var target = avatar.position;
        var distance = Vec3.distance(target, eye);
        var offset = 0.2;
        var diff = Vec3.subtract(target, eye); // get diff between target and eye (a vector pointing to the eye from avatar position)
        var headIndex = avatar.getJointIndex("Head"); // base offset on 1/2 distance from hips to head if we can
        if (headIndex > 0) {
            offset = avatar.getAbsoluteJointTranslationInObjectFrame(headIndex).y / 2;
        }

        // move a bit in front, towards the camera
        target = Vec3.subtract(target, Vec3.multiply(Vec3.normalize(diff), offset));

        // now bump it up a bit
        target.y = target.y + offset;

        overlay.ping = pingPong;
        overlay.editOverlay({
            color: color(ExtendedOverlay.isSelected(id), overlay.hovering),
            position: target,
            dimensions: 0.032 * distance
        });
    });
    pingPong = !pingPong;
    ExtendedOverlay.some(function (overlay) { // Remove any that weren't updated. (User is gone.)
        if (overlay.ping === pingPong) {
            overlay.deleteOverlay();
        }
    });
}
function removeOverlays() {
    selectedId = false;
    lastHoveringId = 0;
    ExtendedOverlay.some(function (overlay) {
        overlay.deleteOverlay();
    });
}

//
// Clicks.
//
function usernameFromIDReply(id, username, machineFingerprint, isAdmin) {
    if (selectedId === id) {
        var message = {
            method: 'updateSelectedRecipientUsername',
            userName: username === "" ? "unknown username" : username
        };
        ui.tablet.sendToQml(message);
    }
}
function handleClick(pickRay) {
    ExtendedOverlay.applyPickRay(pickRay, function (overlay) {
        var nextSelectedStatus = !overlay.selected;
        var avatarId = overlay.key;
        selectedId = nextSelectedStatus ? avatarId : false;
        if (nextSelectedStatus) {
            Users.requestUsernameFromID(avatarId);
        }
        var message = {
            method: 'selectRecipient',
            id: avatarId,
            isSelected: nextSelectedStatus,
            displayName: '"' + AvatarList.getAvatar(avatarId).sessionDisplayName + '"',
            userName: ''
        };
        ui.tablet.sendToQml(message);

        ExtendedOverlay.some(function (overlay) {
            var id = overlay.key;
            var selected = ExtendedOverlay.isSelected(id);
            overlay.select(selected);
        });

        return true;
    });
}
function handleMouseEvent(mousePressEvent) { // handleClick if we get one.
    if (!mousePressEvent.isLeftButton) {
        return;
    }
    handleClick(Camera.computePickRay(mousePressEvent.x, mousePressEvent.y));
}
function handleMouseMove(pickRay) { // given the pickRay, just do the hover logic
    ExtendedOverlay.applyPickRay(pickRay, function (overlay) {
        overlay.hover(true);
    }, function () {
        ExtendedOverlay.unHover();
    });
}

// handy global to keep track of which hand is the mouse (if any)
var currentHandPressed = 0;
var TRIGGER_CLICK_THRESHOLD = 0.85;
var TRIGGER_PRESS_THRESHOLD = 0.05;

function handleMouseMoveEvent(event) { // find out which overlay (if any) is over the mouse position
    var pickRay;
    if (HMD.active) {
        if (currentHandPressed !== 0) {
            pickRay = controllerComputePickRay(currentHandPressed);
        } else {
            // nothing should hover, so
            ExtendedOverlay.unHover();
            return;
        }
    } else {
        pickRay = Camera.computePickRay(event.x, event.y);
    }
    handleMouseMove(pickRay);
}
function handleTriggerPressed(hand, value) {
    // The idea is if you press one trigger, it is the one
    // we will consider the mouse.  Even if the other is pressed,
    // we ignore it until this one is no longer pressed.
    var isPressed = value > TRIGGER_PRESS_THRESHOLD;
    if (currentHandPressed === 0) {
        currentHandPressed = isPressed ? hand : 0;
        return;
    }
    if (currentHandPressed === hand) {
        currentHandPressed = isPressed ? hand : 0;
        return;
    }
    // otherwise, the other hand is still triggered
    // so do nothing.
}

// We get mouseMoveEvents from the handControllers, via handControllerPointer.
// But we don't get mousePressEvents.
var triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');
var triggerPressMapping = Controller.newMapping(Script.resolvePath('') + '-press');
function controllerComputePickRay(hand) {
    var controllerPose = getControllerWorldLocation(hand, true);
    if (controllerPose.valid) {
        return { origin: controllerPose.position, direction: Quat.getUp(controllerPose.orientation) };
    }
}
function makeClickHandler(hand) {
    return function (clicked) {
        if (clicked > TRIGGER_CLICK_THRESHOLD) {
            var pickRay = controllerComputePickRay(hand);
            handleClick(pickRay);
        }
    };
}
function makePressHandler(hand) {
    return function (value) {
        handleTriggerPressed(hand, value);
    };
}
triggerMapping.from(Controller.Standard.RTClick).peek().to(makeClickHandler(Controller.Standard.RightHand));
triggerMapping.from(Controller.Standard.LTClick).peek().to(makeClickHandler(Controller.Standard.LeftHand));
triggerPressMapping.from(Controller.Standard.RT).peek().to(makePressHandler(Controller.Standard.RightHand));
triggerPressMapping.from(Controller.Standard.LT).peek().to(makePressHandler(Controller.Standard.LeftHand));
// END AVATAR SELECTOR LOGIC

var grid = new Grid();
function adjustPositionPerBoundingBox(position, direction, registration, dimensions, orientation) {
    // Adjust the position such that the bounding box (registration, dimenions, and orientation) lies behind the original
    // position in the given direction.
    var CORNERS = [
        { x: 0, y: 0, z: 0 },
        { x: 0, y: 0, z: 1 },
        { x: 0, y: 1, z: 0 },
        { x: 0, y: 1, z: 1 },
        { x: 1, y: 0, z: 0 },
        { x: 1, y: 0, z: 1 },
        { x: 1, y: 1, z: 0 },
        { x: 1, y: 1, z: 1 }
    ];

    // Go through all corners and find least (most negative) distance in front of position.
    var distance = 0;
    for (var i = 0, length = CORNERS.length; i < length; i++) {
        var cornerVector =
            Vec3.multiplyQbyV(orientation, Vec3.multiplyVbyV(Vec3.subtract(CORNERS[i], registration), dimensions));
        var cornerDistance = Vec3.dot(cornerVector, direction);
        distance = Math.min(cornerDistance, distance);
    }
    position = Vec3.sum(Vec3.multiply(distance, direction), position);
    return position;
}

var HALF_TREE_SCALE = 16384;
function getPositionToCreateEntity(extra) {
    var CREATE_DISTANCE = 2;
    var position;
    var delta = extra !== undefined ? extra : 0;
    if (Camera.mode === "entity" || Camera.mode === "independent") {
        position = Vec3.sum(Camera.position, Vec3.multiply(Quat.getForward(Camera.orientation), CREATE_DISTANCE + delta));
    } else {
        position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getForward(MyAvatar.orientation), CREATE_DISTANCE + delta));
        position.y += 0.5;
    }

    if (position.x > HALF_TREE_SCALE || position.y > HALF_TREE_SCALE || position.z > HALF_TREE_SCALE) {
        return null;
    }
    return position;
}

function rezEntity(itemHref, itemType) {
    var isWearable = itemType === "wearable";
    var success = Clipboard.importEntities(itemHref);
    var wearableLocalPosition = null;
    var wearableLocalRotation = null;
    var wearableLocalDimensions = null;
    var wearableDimensions = null;

    if (itemType === "contentSet") {
        console.log("Item is a content set; codepath shouldn't go here.");
        return;
    }

    if (isWearable) {
        var wearableTransforms = Settings.getValue("io.highfidelity.avatarStore.checkOut.transforms");
        if (!wearableTransforms) {
            // TODO delete this clause
            wearableTransforms = Settings.getValue("io.highfidelity.avatarStore.checkOut.tranforms");
        }
        var certPos = itemHref.search("certificate_id="); // TODO how do I parse a URL from here?
        if (certPos >= 0) {
            certPos += 15; // length of "certificate_id="
            var certURLEncoded = itemHref.substring(certPos);
            var certB64Encoded = decodeURIComponent(certURLEncoded);
            for (var key in wearableTransforms) {
                if (wearableTransforms.hasOwnProperty(key)) {
                    var certificateTransforms = wearableTransforms[key].certificateTransforms;
                    if (certificateTransforms) {
                        for (var certID in certificateTransforms) {
                            if (certificateTransforms.hasOwnProperty(certID) &&
                                certID == certB64Encoded) {
                                var certificateTransform = certificateTransforms[certID];
                                wearableLocalPosition = certificateTransform.localPosition;
                                wearableLocalRotation = certificateTransform.localRotation;
                                wearableLocalDimensions = certificateTransform.localDimensions;
                                wearableDimensions = certificateTransform.dimensions;
                            }
                        }
                    }
                }
            }
        }
    }

    if (success) {
        var VERY_LARGE = 10000;
        var isLargeImport = Clipboard.getClipboardContentsLargestDimension() >= VERY_LARGE;
        var position = Vec3.ZERO;
        if (!isLargeImport) {
            position = getPositionToCreateEntity(Clipboard.getClipboardContentsLargestDimension() / 2);
        }
        if (position !== null && position !== undefined) {
            var pastedEntityIDs = Clipboard.pasteEntities(position);
            if (!isLargeImport) {
                // The first entity in Clipboard gets the specified position with the rest being relative to it. Therefore, move
                // entities after they're imported so that they're all the correct distance in front of and with geometric mean
                // centered on the avatar/camera direction.
                var deltaPosition = Vec3.ZERO;
                var entityPositions = [];
                var entityParentIDs = [];

                var propType = Entities.getEntityProperties(pastedEntityIDs[0], ["type"]).type;
                var NO_ADJUST_ENTITY_TYPES = ["Zone", "Light", "ParticleEffect"];
                if (NO_ADJUST_ENTITY_TYPES.indexOf(propType) === -1) {
                    var targetDirection;
                    if (Camera.mode === "entity" || Camera.mode === "independent") {
                        targetDirection = Camera.orientation;
                    } else {
                        targetDirection = MyAvatar.orientation;
                    }
                    targetDirection = Vec3.multiplyQbyV(targetDirection, Vec3.UNIT_Z);

                    var targetPosition = getPositionToCreateEntity();
                    var deltaParallel = HALF_TREE_SCALE;  // Distance to move entities parallel to targetDirection.
                    var deltaPerpendicular = Vec3.ZERO;  // Distance to move entities perpendicular to targetDirection.
                    for (var i = 0, length = pastedEntityIDs.length; i < length; i++) {
                        var curLoopEntityProps = Entities.getEntityProperties(pastedEntityIDs[i], ["position", "dimensions",
                            "registrationPoint", "rotation", "parentID"]);
                        var adjustedPosition = adjustPositionPerBoundingBox(targetPosition, targetDirection,
                            curLoopEntityProps.registrationPoint, curLoopEntityProps.dimensions, curLoopEntityProps.rotation);
                        var delta = Vec3.subtract(adjustedPosition, curLoopEntityProps.position);
                        var distance = Vec3.dot(delta, targetDirection);
                        deltaParallel = Math.min(distance, deltaParallel);
                        deltaPerpendicular = Vec3.sum(Vec3.subtract(delta, Vec3.multiply(distance, targetDirection)),
                            deltaPerpendicular);
                        entityPositions[i] = curLoopEntityProps.position;
                        entityParentIDs[i] = curLoopEntityProps.parentID;
                    }
                    deltaPerpendicular = Vec3.multiply(1 / pastedEntityIDs.length, deltaPerpendicular);
                    deltaPosition = Vec3.sum(Vec3.multiply(deltaParallel, targetDirection), deltaPerpendicular);
                }

                if (grid.getSnapToGrid()) {
                    var firstEntityProps = Entities.getEntityProperties(pastedEntityIDs[0], ["position", "dimensions",
                        "registrationPoint"]);
                    var positionPreSnap = Vec3.sum(deltaPosition, firstEntityProps.position);
                    position = grid.snapToSurface(grid.snapToGrid(positionPreSnap, false, firstEntityProps.dimensions,
                        firstEntityProps.registrationPoint), firstEntityProps.dimensions, firstEntityProps.registrationPoint);
                    deltaPosition = Vec3.subtract(position, firstEntityProps.position);
                }

                if (!Vec3.equal(deltaPosition, Vec3.ZERO)) {
                    for (var editEntityIndex = 0, numEntities = pastedEntityIDs.length; editEntityIndex < numEntities; editEntityIndex++) {
                        if (Uuid.isNull(entityParentIDs[editEntityIndex])) {
                            Entities.editEntity(pastedEntityIDs[editEntityIndex], {
                                position: Vec3.sum(deltaPosition, entityPositions[editEntityIndex])
                            });
                        }
                    }
                }
            }

            if (isWearable) {
                // apply the relative offsets saved during checkout
                var offsets = {};
                if (wearableLocalPosition) {
                    offsets.localPosition = wearableLocalPosition;
                }
                if (wearableLocalRotation) {
                    offsets.localRotation = wearableLocalRotation;
                }
                if (wearableLocalDimensions) {
                    offsets.localDimensions = wearableLocalDimensions;
                } else if (wearableDimensions) {
                    offsets.dimensions = wearableDimensions;
                }
                // we currently assume a wearable is a single entity
                Entities.editEntity(pastedEntityIDs[0], offsets);
            }

            var rezPosition = Entities.getEntityProperties(pastedEntityIDs[0], "position").position;

            Audio.playSound(REZZING_SOUND, {
                volume: 1.0,
                position: rezPosition,
                localOnly: true
            });

        } else {
            Window.notifyEditError("Can't import entities: entities would be out of bounds.");
        }
    } else {
        Window.notifyEditError("There was an error importing the entity file.");
    }
}

var referrerURL; // Used for updating Purchases QML
var filterText; // Used for updating Purchases QML
function onWebEventReceived(message) {
    message = JSON.parse(message);
    if (message.type === GOTO_DIRECTORY) {
        ui.open(MARKETPLACES_URL, MARKETPLACES_INJECT_SCRIPT_URL);
    } else if (message.type === QUERY_CAN_WRITE_ASSETS) {
        ui.sendToHtml(CAN_WRITE_ASSETS + " " + Entities.canWriteAssets());
    } else if (message.type === WARN_USER_NO_PERMISSIONS) {
        Window.alert(NO_PERMISSIONS_ERROR_MESSAGE);
    } else if (message.type === CLARA_IO_STATUS) {
        if (isDownloadBeingCancelled) {
            return;
        }

        var text = message.status;
        if (messageBox === null) {
            messageBox = Window.openMessageBox(CLARA_DOWNLOAD_TITLE, text, CANCEL_BUTTON, NO_BUTTON);
        } else {
            Window.updateMessageBox(messageBox, CLARA_DOWNLOAD_TITLE, text, CANCEL_BUTTON, NO_BUTTON);
        }
        return;
    } else if (message.type === CLARA_IO_DOWNLOAD) {
        if (messageBox !== null) {
            Window.closeMessageBox(messageBox);
            messageBox = null;
        }
        return;
    } else if (message.type === CLARA_IO_CANCELLED_DOWNLOAD) {
        isDownloadBeingCancelled = false;
    } else if (message.type === "CHECKOUT") {
        wireQmlEventBridge(true);
        ui.open(MARKETPLACE_CHECKOUT_QML_PATH);
        ui.tablet.sendToQml({
            method: 'updateCheckoutQML',
            params: message
        });
    } else if (message.type === "REQUEST_SETTING") {
        sendCommerceSettings();
    } else if (message.type === "PURCHASES") {
        referrerURL = message.referrerURL;
        filterText = "";
        ui.open(MARKETPLACE_PURCHASES_QML_PATH);
    } else if (message.type === "LOGIN") {
        openLoginWindow();
    } else if (message.type === "WALLET_SETUP") {
        wireQmlEventBridge(true);
        ui.tablet.sendToQml({
            method: 'updateWalletReferrer',
            referrer: "marketplace cta"
        });
        openWallet();
    } else if (message.type === "MY_ITEMS") {
        referrerURL = MARKETPLACE_URL_INITIAL;
        filterText = "";
        ui.open(MARKETPLACE_PURCHASES_QML_PATH);
        wireQmlEventBridge(true);
        ui.tablet.sendToQml({
            method: 'purchases_showMyItems'
        });
    }
}
var sendAssetRecipient;
var sendAssetParticleEffectUpdateTimer;
var particleEffectTimestamp;
var sendAssetParticleEffect;
var SEND_ASSET_PARTICLE_TIMER_UPDATE = 250;
var SEND_ASSET_PARTICLE_EMITTING_DURATION = 3000;
var SEND_ASSET_PARTICLE_LIFETIME_SECONDS = 8;
var SEND_ASSET_PARTICLE_PROPERTIES = {
    accelerationSpread: { x: 0, y: 0, z: 0 },
    alpha: 1,
    alphaFinish: 1,
    alphaSpread: 0,
    alphaStart: 1,
    azimuthFinish: 0,
    azimuthStart: -6,
    color: { red: 255, green: 222, blue: 255 },
    colorFinish: { red: 255, green: 229, blue: 225 },
    colorSpread: { red: 0, green: 0, blue: 0 },
    colorStart: { red: 243, green: 255, blue: 255 },
    emitAcceleration: { x: 0, y: 0, z: 0 }, // Immediately gets updated to be accurate
    emitDimensions: { x: 0, y: 0, z: 0 },
    emitOrientation: { x: 0, y: 0, z: 0 },
    emitRate: 4,
    emitSpeed: 2.1,
    emitterShouldTrail: true,
    isEmitting: 1,
    lifespan: SEND_ASSET_PARTICLE_LIFETIME_SECONDS + 1, // Immediately gets updated to be accurate
    lifetime: SEND_ASSET_PARTICLE_LIFETIME_SECONDS + 1,
    maxParticles: 20,
    name: 'asset-particles',
    particleRadius: 0.2,
    polarFinish: 0,
    polarStart: 0,
    radiusFinish: 0.05,
    radiusSpread: 0,
    radiusStart: 0.2,
    speedSpread: 0,
    textures: "http://hifi-content.s3.amazonaws.com/alan/dev/Particles/Bokeh-Particle-HFC.png",
    type: 'ParticleEffect'
};

function updateSendAssetParticleEffect() {
    var timestampNow = Date.now();
    if ((timestampNow - particleEffectTimestamp) > (SEND_ASSET_PARTICLE_LIFETIME_SECONDS * 1000)) {
        deleteSendAssetParticleEffect();
        return;
    } else if ((timestampNow - particleEffectTimestamp) > SEND_ASSET_PARTICLE_EMITTING_DURATION) {
        Entities.editEntity(sendAssetParticleEffect, {
            isEmitting: 0
        });
    } else if (sendAssetParticleEffect) {
        var recipientPosition = AvatarList.getAvatar(sendAssetRecipient).position;
        var distance = Vec3.distance(recipientPosition, MyAvatar.position);
        var accel = Vec3.subtract(recipientPosition, MyAvatar.position);
        accel.y -= 3.0;
        var life = Math.sqrt(2 * distance / Vec3.length(accel));
        Entities.editEntity(sendAssetParticleEffect, {
            emitAcceleration: accel,
            lifespan: life
        });
    }
}

function deleteSendAssetParticleEffect() {
    if (sendAssetParticleEffectUpdateTimer) {
        Script.clearInterval(sendAssetParticleEffectUpdateTimer);
        sendAssetParticleEffectUpdateTimer = null;
    }
    if (sendAssetParticleEffect) {
        sendAssetParticleEffect = Entities.deleteEntity(sendAssetParticleEffect);
    }
    sendAssetRecipient = null;
}
    
var savedDisablePreviewOption = Menu.isOptionChecked("Disable Preview");
var UI_FADE_TIMEOUT_MS = 150;
function maybeEnableHMDPreview() {
    // Set a small timeout to prevent sensitive data from being shown during UI fade
    Script.setTimeout(function () {
        setTabletVisibleInSecondaryCamera(true);
        DesktopPreviewProvider.setPreviewDisabledReason("USER");
        Menu.setIsOptionChecked("Disable Preview", savedDisablePreviewOption);
    }, UI_FADE_TIMEOUT_MS);
}

var onQmlMessageReceived = function onQmlMessageReceived(message) {
    if (message.messageSrc === "HTML") {
        return;
    }
    switch (message.method) {
    case 'purchases_openWallet':
    case 'checkout_openWallet':
    case 'checkout_setUpClicked':
        openWallet();
        break;
    case 'purchases_walletNotSetUp':
        wireQmlEventBridge(true);
        ui.tablet.sendToQml({
            method: 'updateWalletReferrer',
            referrer: "purchases"
        });
        openWallet();
        break;
    case 'checkout_walletNotSetUp':
        wireQmlEventBridge(true);
        ui.tablet.sendToQml({
            method: 'updateWalletReferrer',
            referrer: message.referrer === "itemPage" ? message.itemId : message.referrer
        });
        openWallet();
        break;
    case 'checkout_cancelClicked':
        ui.open(MARKETPLACE_URL + '/items/' + message.params, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'header_goToPurchases':
    case 'checkout_goToPurchases':
        referrerURL = MARKETPLACE_URL_INITIAL;
        filterText = message.filterText;
        ui.open(MARKETPLACE_PURCHASES_QML_PATH);
        break;
    case 'checkout_itemLinkClicked':
        ui.open(MARKETPLACE_URL + '/items/' + message.itemId, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'checkout_continueShopping':
        ui.open(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'purchases_itemInfoClicked':
        var itemId = message.itemId;
        if (itemId && itemId !== "") {
            ui.open(MARKETPLACE_URL + '/items/' + itemId, MARKETPLACES_INJECT_SCRIPT_URL);
        }
        break;
    case 'checkout_rezClicked':
    case 'purchases_rezClicked':
        rezEntity(message.itemHref, message.itemType);
        break;
    case 'header_marketplaceImageClicked':
    case 'purchases_backClicked':
        ui.open(message.referrerURL, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'purchases_goToMarketplaceClicked':
        ui.open(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'updateItemClicked':
        ui.open(message.upgradeUrl + "?edition=" + message.itemEdition,
            MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'giftAsset':

        break;
    case 'passphrasePopup_cancelClicked':
    case 'needsLogIn_cancelClicked':
        ui.open(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'needsLogIn_loginClicked':
        openLoginWindow();
        break;
    case 'disableHmdPreview':
        if (!savedDisablePreviewOption) {
            savedDisablePreviewOption = Menu.isOptionChecked("Disable Preview");
        }

        if (!savedDisablePreviewOption) {
            DesktopPreviewProvider.setPreviewDisabledReason("SECURE_SCREEN");
            Menu.setIsOptionChecked("Disable Preview", true);
            setTabletVisibleInSecondaryCamera(false);
        }
        break;
    case 'maybeEnableHmdPreview':
        maybeEnableHMDPreview();
        break;
    case 'purchases_openGoTo':
        ui.open("hifi/tablet/TabletAddressDialog.qml");
        break;
    case 'purchases_itemCertificateClicked':
        contextOverlayEntity = "";
        setCertificateInfo(contextOverlayEntity, message.itemCertificateId);
        break;
    case 'inspectionCertificate_closeClicked':
        ui.close();
        break;
    case 'inspectionCertificate_requestOwnershipVerification':
        ContextOverlay.requestOwnershipVerification(message.entity);
        break;
    case 'inspectionCertificate_showInMarketplaceClicked':
        ui.open(message.marketplaceUrl, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'header_myItemsClicked':
        referrerURL = MARKETPLACE_URL_INITIAL;
        filterText = "";
        ui.open(MARKETPLACE_PURCHASES_QML_PATH);
        wireQmlEventBridge(true);
        ui.tablet.sendToQml({
            method: 'purchases_showMyItems'
        });
        break;
    case 'refreshConnections':
        // Guard to prevent this code from being executed while sending money --
        // we only want to execute this while sending non-HFC gifts
        if (!onWalletScreen) {
            print('Refreshing Connections...');
            getConnectionData(false);
        }
        break;
    case 'enable_ChooseRecipientNearbyMode':
        // Guard to prevent this code from being executed while sending money --
        // we only want to execute this while sending non-HFC gifts
        if (!onWalletScreen) {
            if (!isUpdateOverlaysWired) {
                Script.update.connect(updateOverlays);
                isUpdateOverlaysWired = true;
            }
        }
        break;
    case 'disable_ChooseRecipientNearbyMode':
        // Guard to prevent this code from being executed while sending money --
        // we only want to execute this while sending non-HFC gifts
        if (!onWalletScreen) {
            if (isUpdateOverlaysWired) {
                Script.update.disconnect(updateOverlays);
                isUpdateOverlaysWired = false;
            }
            removeOverlays();
        }
        break;
    case 'wallet_availableUpdatesReceived':
    case 'purchases_availableUpdatesReceived':
        userHasUpdates = message.numUpdates > 0;
        ui.messagesWaiting(userHasUpdates);
        break;
    case 'purchases_updateWearables':
        var currentlyWornWearables = [];
        var ATTACHMENT_SEARCH_RADIUS = 100; // meters (just in case)

        var nearbyEntities = Entities.findEntitiesByType('Model', MyAvatar.position, ATTACHMENT_SEARCH_RADIUS);

        for (var i = 0; i < nearbyEntities.length; i++) {
            var currentProperties = Entities.getEntityProperties(
                nearbyEntities[i], ['certificateID', 'editionNumber', 'parentID']
            );
            if (currentProperties.parentID === MyAvatar.sessionUUID) {
                currentlyWornWearables.push({
                    entityID: nearbyEntities[i],
                    entityCertID: currentProperties.certificateID,
                    entityEdition: currentProperties.editionNumber
                });
            }
        }

        ui.tablet.sendToQml({ method: 'updateWearables', wornWearables: currentlyWornWearables });
        break;
    case 'sendAsset_sendPublicly':
        if (message.assetName !== "") {
            deleteSendAssetParticleEffect();
            sendAssetRecipient = message.recipient;
            var props = SEND_ASSET_PARTICLE_PROPERTIES;
            props.parentID = MyAvatar.sessionUUID;
            props.position = MyAvatar.position;
            props.position.y += 0.2;
            if (message.effectImage) {
                props.textures = message.effectImage;
            }
            sendAssetParticleEffect = Entities.addEntity(props, true);
            particleEffectTimestamp = Date.now();
            updateSendAssetParticleEffect();
            sendAssetParticleEffectUpdateTimer = Script.setInterval(updateSendAssetParticleEffect,
                SEND_ASSET_PARTICLE_TIMER_UPDATE);
        }
        break;
    case 'http.request':
        // Handled elsewhere, don't log.
        break;
    case 'goToPurchases_fromWalletHome': // HRS FIXME What's this about?
        break;
    default:
        print('Unrecognized message from Checkout.qml or Purchases.qml: ' + JSON.stringify(message));
    }
};

// Function Name: onTabletScreenChanged()
//
// Description:
//   -Called when the TabletScriptingInterface::screenChanged() signal is emitted. The "type" argument can be either the string
//    value of "Home", "Web", "Menu", "QML", or "Closed". The "url" argument is only valid for Web and QML.
var onMarketplaceScreen = false;
var onWalletScreen = false;
var onCommerceScreen = false;
var onInspectionCertificateScreen = false;
var onTabletScreenChanged = function onTabletScreenChanged(type, url) {
    ui.setCurrentVisibleScreenMetadata(type, url);
    onMarketplaceScreen = type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1;
    onInspectionCertificateScreen = type === "QML" && url.indexOf(MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH) !== -1;
    var onWalletScreenNow = url.indexOf(MARKETPLACE_WALLET_QML_PATH) !== -1;
    var onCommerceScreenNow = type === "QML" &&
        (url.indexOf(MARKETPLACE_CHECKOUT_QML_PATH) !== -1 || url === MARKETPLACE_PURCHASES_QML_PATH ||
        onInspectionCertificateScreen);

    // exiting wallet or commerce screen
    if ((!onWalletScreenNow && onWalletScreen) || (!onCommerceScreenNow && onCommerceScreen)) {
        maybeEnableHMDPreview();
    }

    onCommerceScreen = onCommerceScreenNow;
    onWalletScreen = onWalletScreenNow;
    wireQmlEventBridge(onMarketplaceScreen || onCommerceScreen || onWalletScreen);

    if (url === MARKETPLACE_PURCHASES_QML_PATH) {
        ui.tablet.sendToQml({
            method: 'updatePurchases',
            referrerURL: referrerURL,
            filterText: filterText
        });
    }

    ui.isOpen = (onMarketplaceScreen || onCommerceScreen) && !onWalletScreen;
    ui.buttonActive(ui.isOpen);

    if (type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1) {
        ContextOverlay.isInMarketplaceInspectionMode = true;
    } else {
        ContextOverlay.isInMarketplaceInspectionMode = false;
    }

    if (onInspectionCertificateScreen) {
        setCertificateInfo(contextOverlayEntity);
    }

    if (onCommerceScreen) {
        if (!isWired) {
            Users.usernameFromIDReply.connect(usernameFromIDReply);
            Controller.mousePressEvent.connect(handleMouseEvent);
            Controller.mouseMoveEvent.connect(handleMouseMoveEvent);
            triggerMapping.enable();
            triggerPressMapping.enable();
        }
        isWired = true;
        Wallet.refreshWalletStatus();
    } else {
        ui.tablet.sendToQml({
            method: 'inspectionCertificate_resetCert'
        });
        off();
    }
    console.debug(ui.buttonName + " app reports: Tablet screen changed.\nNew screen type: " + type +
        "\nNew screen URL: " + url + "\nCurrent app open status: " + ui.isOpen + "\n");
};

//
// Manage the connection between the button and the window.
//
var BUTTON_NAME = "MARKET";
var MARKETPLACE_URL = METAVERSE_SERVER_URL + "/marketplace";
var MARKETPLACE_URL_INITIAL = MARKETPLACE_URL + "?"; // Append "?" to signal injected script that it's the initial page.
var ui;
function startup() {
    ui = new AppUi({
        buttonName: BUTTON_NAME,
        sortOrder: 9,
        inject: MARKETPLACES_INJECT_SCRIPT_URL,
        home: MARKETPLACE_URL_INITIAL,
        onScreenChanged: onTabletScreenChanged,
        onMessage: onQmlMessageReceived
    });
    ContextOverlay.contextOverlayClicked.connect(openInspectionCertificateQML);
    Entities.canWriteAssetsChanged.connect(onCanWriteAssetsChanged);
    GlobalServices.myUsernameChanged.connect(onUsernameChanged);
    ui.tablet.webEventReceived.connect(onWebEventReceived);
    Wallet.walletStatusChanged.connect(sendCommerceSettings);
    Window.messageBoxClosed.connect(onMessageBoxClosed);

    Wallet.refreshWalletStatus();
}

var isWired = false;
var isUpdateOverlaysWired = false;
function off() {
    if (isWired) {
        Users.usernameFromIDReply.disconnect(usernameFromIDReply);
        Controller.mousePressEvent.disconnect(handleMouseEvent);
        Controller.mouseMoveEvent.disconnect(handleMouseMoveEvent);
        triggerMapping.disable();
        triggerPressMapping.disable();

        isWired = false;
    }

    if (isUpdateOverlaysWired) {
        Script.update.disconnect(updateOverlays);
        isUpdateOverlaysWired = false;
    }
    removeOverlays();
}
function shutdown() {
    maybeEnableHMDPreview();
    deleteSendAssetParticleEffect();

    Window.messageBoxClosed.disconnect(onMessageBoxClosed);
    Wallet.walletStatusChanged.disconnect(sendCommerceSettings);
    ui.tablet.webEventReceived.disconnect(onWebEventReceived);
    GlobalServices.myUsernameChanged.disconnect(onUsernameChanged);
    Entities.canWriteAssetsChanged.disconnect(onCanWriteAssetsChanged);
    ContextOverlay.contextOverlayClicked.disconnect(openInspectionCertificateQML);

    off();
}

//
// Run the functions.
//
startup();
Script.scriptEnding.connect(shutdown);
}()); // END LOCAL_SCOPE
