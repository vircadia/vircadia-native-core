"use strict";
/* jslint vars:true, plusplus:true, forin:true */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// wallet.js
//
// Created by Zach Fox on 2017-08-17
// Copyright 2017 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global getConnectionData getControllerWorldLocation openLoginWindow WalletScriptingInterface */

(function () { // BEGIN LOCAL_SCOPE
Script.include("/~/system/libraries/accountUtils.js");
Script.include("/~/system/libraries/connectionUtils.js");
var AppUi = Script.require('appUi');

var MARKETPLACE_URL = Account.metaverseServerURL + "/marketplace";

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
    // Depends on nearer coverOverlays to extend closer to us than farther ones.
    var pickedOverlay = Overlays.findRayIntersection(pickRay);
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
var OVERLAY_SCALE = 0.032;
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
        // get diff between target and eye (a vector pointing to the eye from avatar position)
        var diff = Vec3.subtract(target, eye);
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
            dimensions: OVERLAY_SCALE * distance
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
        ui.sendMessage(message);
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
        ui.sendMessage(message);

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

var sendMoneyRecipient;
var sendMoneyParticleEffectUpdateTimer;
var particleEffectTimestamp;
var sendMoneyParticleEffect;
var SEND_MONEY_PARTICLE_TIMER_UPDATE = 250;
var SEND_MONEY_PARTICLE_EMITTING_DURATION = 3000;
var SEND_MONEY_PARTICLE_LIFETIME_SECONDS = 8;
var SEND_MONEY_PARTICLE_PROPERTIES = {
    accelerationSpread: { x: 0, y: 0, z: 0 },
    alpha: 1,
    alphaFinish: 1,
    alphaSpread: 0,
    alphaStart: 1,
    azimuthFinish: 0,
    azimuthStart: -6,
    color: { red: 143, green: 5, blue: 255 },
    colorFinish: { red: 255, green: 0, blue: 204 },
    colorSpread: { red: 0, green: 0, blue: 0 },
    colorStart: { red: 0, green: 136, blue: 255 },
    emitAcceleration: { x: 0, y: 0, z: 0 }, // Immediately gets updated to be accurate
    emitDimensions: { x: 0, y: 0, z: 0 },
    emitOrientation: { x: 0, y: 0, z: 0 },
    emitRate: 4,
    emitSpeed: 2.1,
    emitterShouldTrail: true,
    isEmitting: 1,
    lifespan: SEND_MONEY_PARTICLE_LIFETIME_SECONDS + 1, // Immediately gets updated to be accurate
    lifetime: SEND_MONEY_PARTICLE_LIFETIME_SECONDS + 1,
    maxParticles: 20,
    name: 'hfc-particles',
    particleRadius: 0.2,
    polarFinish: 0,
    polarStart: 0,
    radiusFinish: 0.05,
    radiusSpread: 0,
    radiusStart: 0.2,
    speedSpread: 0,
    textures: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/alan/dev/Particles/Bokeh-Particle-HFC.png"),
    type: 'ParticleEffect'
};

var MS_PER_SEC = 1000;
function updateSendMoneyParticleEffect() {
    var timestampNow = Date.now();
    if ((timestampNow - particleEffectTimestamp) > (SEND_MONEY_PARTICLE_LIFETIME_SECONDS * MS_PER_SEC)) {
        deleteSendMoneyParticleEffect();
        return;
    } else if ((timestampNow - particleEffectTimestamp) > SEND_MONEY_PARTICLE_EMITTING_DURATION) {
        Entities.editEntity(sendMoneyParticleEffect, {
            isEmitting: 0
        });
    } else if (sendMoneyParticleEffect) {
        var recipientPosition = AvatarList.getAvatar(sendMoneyRecipient).position;
        var distance = Vec3.distance(recipientPosition, MyAvatar.position);
        var accel = Vec3.subtract(recipientPosition, MyAvatar.position);
        accel.y -= 3.0;
        var life = Math.sqrt(2 * distance / Vec3.length(accel));
        Entities.editEntity(sendMoneyParticleEffect, {
            emitAcceleration: accel,
            lifespan: life
        });
    }
}

function deleteSendMoneyParticleEffect() {
    if (sendMoneyParticleEffectUpdateTimer) {
        Script.clearInterval(sendMoneyParticleEffectUpdateTimer);
        sendMoneyParticleEffectUpdateTimer = null;
    }
    if (sendMoneyParticleEffect) {
        sendMoneyParticleEffect = Entities.deleteEntity(sendMoneyParticleEffect);
    }
    sendMoneyRecipient = null;
}

function onUsernameChanged() {
    if (ui.checkIsOpen()) {
        ui.open(WALLET_QML_SOURCE);
    }
}    

var MARKETPLACE_QML_PATH = "hifi/commerce/marketplace/Marketplace.qml";
function openMarketplace(optionalItem) {
    ui.open(MARKETPLACE_QML_PATH);
    
    if (optionalItem) {
        ui.tablet.sendToQml({
            method: 'updateMarketplaceQMLItem',
            params: { itemId: optionalItem }
        });
    }
}

function setCertificateInfo(itemCertificateId) {
    ui.tablet.sendToQml({
        method: 'inspectionCertificate_setCertificateId',
        entityId: "",
        certificateId: itemCertificateId
    });
}

// Function Name: fromQml()
//
// Description:
//   -Called when a message is received from SpectatorCamera.qml. The "message" argument is what is sent from the QML
//    in the format "{method, params}", like json-rpc. See also sendToQml().
function fromQml(message) {
    switch (message.method) {
    case 'passphrasePopup_cancelClicked':
    case 'needsLogIn_cancelClicked':
        ui.close();
        break;
    case 'walletSetup_cancelClicked':
        switch (message.referrer) {
        case '': // User clicked "Wallet" app
        case undefined:
        case null:
            ui.close();
            break;
        case 'purchases':
        case 'marketplace cta':
        case 'mainPage':
            openMarketplace();
            break;
        default:
            openMarketplace();
            break;
        }
        break;
    case 'needsLogIn_loginClicked':
        openLoginWindow();
        break;
    case 'disableHmdPreview':
        break; // do nothing here, handled in marketplaces.js
    case 'maybeEnableHmdPreview':
        break; // do nothing here, handled in marketplaces.js
    case 'transactionHistory_linkClicked':
        openMarketplace(message.itemId);
        break;
    case 'goToMarketplaceMainPage':
        openMarketplace();
        break;
    case 'goToMarketplaceItemPage':
        openMarketplace(message.itemId);
        break;
    case 'refreshConnections':
        print('Refreshing Connections...');
        getConnectionData(false);
        break;
    case 'enable_ChooseRecipientNearbyMode':
        if (!isUpdateOverlaysWired) {
            Script.update.connect(updateOverlays);
            isUpdateOverlaysWired = true;
        }
        break;
    case 'disable_ChooseRecipientNearbyMode':
        if (isUpdateOverlaysWired) {
            Script.update.disconnect(updateOverlays);
            isUpdateOverlaysWired = false;
        }
        removeOverlays();
        break;
    case 'sendAsset_sendPublicly':
        deleteSendMoneyParticleEffect();
        sendMoneyRecipient = message.recipient;
        var props = SEND_MONEY_PARTICLE_PROPERTIES;
        props.parentID = MyAvatar.sessionUUID;
        props.position = MyAvatar.position;
        props.position.y += 0.2;
        if (message.effectImage) {
            props.textures = message.effectImage;
        }
        sendMoneyParticleEffect = Entities.addEntity(props, true);
        particleEffectTimestamp = Date.now();
        updateSendMoneyParticleEffect();
        sendMoneyParticleEffectUpdateTimer =
            Script.setInterval(updateSendMoneyParticleEffect, SEND_MONEY_PARTICLE_TIMER_UPDATE);
        break;
    case 'transactionHistory_goToBank':
        if (Account.metaverseServerURL.indexOf("staging") >= 0) {
            Window.location = "hifi://hifiqa-master-metaverse-staging"; // So that we can test in staging.
        } else {
            Window.location = "hifi://BankOfHighFidelity";
        }
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
    case 'purchases_walletNotSetUp':
        ui.tablet.sendToQml({
            method: 'updateWalletReferrer',
            referrer: "purchases"
        });
        break;
    case 'purchases_openGoTo':
        ui.open("hifi/tablet/TabletAddressDialog.qml");
        break;
    case 'purchases_itemInfoClicked':
        var itemId = message.itemId;
        if (itemId && itemId !== "") {
            openMarketplace(itemId);
        }
        break;
    case 'purchases_itemCertificateClicked':
        setCertificateInfo(message.itemCertificateId);
        break;
    case 'clearShouldShowDotHistory':
        shouldShowDotHistory = false;
        ui.messagesWaiting(shouldShowDotUpdates || shouldShowDotHistory);
        break;
    case 'clearShouldShowDotUpdates':
        shouldShowDotUpdates = false;
        ui.messagesWaiting(shouldShowDotUpdates || shouldShowDotHistory);
        break;
    case 'http.request':
        // Handled elsewhere, don't log.
        break;
    case 'closeSendAsset':
        ui.close();
        break;
    default:
        print('wallet.js: Unrecognized message from QML');
    }
}

var isWired = false;
function walletOpened() {
    Users.usernameFromIDReply.connect(usernameFromIDReply);
    Controller.mousePressEvent.connect(handleMouseEvent);
    Controller.mouseMoveEvent.connect(handleMouseMoveEvent);
    triggerMapping.enable();
    triggerPressMapping.enable();
    isWired = true;

    if (shouldShowDotHistory) {
        ui.sendMessage({
            method: 'updateRecentActivityMessageLight',
            messagesWaiting: shouldShowDotHistory
        });
    }
}

function walletClosed() {
    off();
}

function notificationDataProcessPageUpdates(data) {
    return data.data.updates;
}

function notificationDataProcessPageHistory(data) {
    return data.data.history;
}

var shouldShowDotUpdates = false;
function notificationPollCallbackUpdates(updatesArray) {
    shouldShowDotUpdates = updatesArray.length > 0;
    ui.messagesWaiting(shouldShowDotUpdates || shouldShowDotHistory);

    if (updatesArray.length > 0) {
        var message;
        if (!ui.notificationInitialCallbackMade[0]) {
            message = updatesArray.length + " of your purchased items " +
                (updatesArray.length === 1 ? "has an update " : "have updates ") +
                "available. Open INVENTORY to update.";
            ui.notificationDisplayBanner(message);

            ui.notificationPollCaresAboutSince[0] = true;
        } else {
            for (var i = 0; i < updatesArray.length; i++) {
                message = "Update available for \"" +
                    updatesArray[i].base_item_title + "\"." +
                    "Open INVENTORY to update.";
                ui.notificationDisplayBanner(message);
            }
        }
    }
}
var shouldShowDotHistory = false;
function notificationPollCallbackHistory(historyArray) {
    if (!ui.isOpen) {
        var notificationCount = historyArray.length;
        shouldShowDotHistory = shouldShowDotHistory || notificationCount > 0;
        ui.messagesWaiting(shouldShowDotUpdates || shouldShowDotHistory);

        if (notificationCount > 0) {
            var message;
            if (!ui.notificationInitialCallbackMade[1]) {
                message = "You have " + notificationCount + " unread recent " +
                    "transaction" + (notificationCount === 1 ? "" : "s") + ". Open INVENTORY to see all activity.";
                ui.notificationDisplayBanner(message);
            } else {
                for (var i = 0; i < notificationCount; i++) {
                    var historyMessage = historyArray[i].message;
                    var sanitizedHistoryMessage = historyMessage.replace(/<\/?[^>]+(>|$)/g, "");
                    message = '"' + sanitizedHistoryMessage + '" ' +
                        "Open INVENTORY to see all activity.";
                    ui.notificationDisplayBanner(message);
                }
            }
        }
    }
}

function isReturnedDataEmptyUpdates(data) {
    var updatesArray = data.data.updates;
    return updatesArray.length === 0;
}

function isReturnedDataEmptyHistory(data) {
    var historyArray = data.data.history;
    return historyArray.length === 0;
}

var DEVELOPER_MENU = "Developer";
var MARKETPLACE_ITEM_TESTER_LABEL = "Marketplace Item Tester";
var MARKETPLACE_ITEM_TESTER_QML_SOURCE = "hifi/commerce/marketplaceItemTester/MarketplaceItemTester.qml";
function installMarketplaceItemTester() {
    if (!Menu.menuExists(DEVELOPER_MENU)) {
        Menu.addMenu(DEVELOPER_MENU);
    }
    if (!Menu.menuItemExists(DEVELOPER_MENU, MARKETPLACE_ITEM_TESTER_LABEL)) {
        Menu.addMenuItem({
            menuName: DEVELOPER_MENU,
            menuItemName: MARKETPLACE_ITEM_TESTER_LABEL,
            isCheckable: false
        });
    }

    Menu.menuItemEvent.connect(function (menuItem) {
        if (menuItem === MARKETPLACE_ITEM_TESTER_LABEL) {
            ui.open(MARKETPLACE_ITEM_TESTER_QML_SOURCE);
        }
    });
}

function uninstallMarketplaceItemTester() {
    if (Menu.menuExists(DEVELOPER_MENU) &&
        Menu.menuItemExists(DEVELOPER_MENU, MARKETPLACE_ITEM_TESTER_LABEL)
    ) {
        Menu.removeMenuItem(DEVELOPER_MENU, MARKETPLACE_ITEM_TESTER_LABEL);
    }
}

var BUTTON_NAME = "INVENTORY";
var WALLET_QML_SOURCE = "hifi/commerce/wallet/Wallet.qml";
var SENDASSET_QML_SOURCE = "hifi/commerce/common/sendAsset/SendAsset.qml";
var NOTIFICATION_POLL_TIMEOUT = 300000;
var ui;
function startup() {
    var notificationPollEndpointArray = ["/api/v1/commerce/available_updates?per_page=10"];
    var notificationPollTimeoutMsArray = [NOTIFICATION_POLL_TIMEOUT];
    var notificationDataProcessPageArray = [notificationDataProcessPageUpdates];
    var notificationPollCallbackArray = [notificationPollCallbackUpdates];
    var notificationPollStopPaginatingConditionMetArray = [isReturnedDataEmptyUpdates];
    var notificationPollCaresAboutSinceArray = [false];

    if (!WalletScriptingInterface.limitedCommerce) {
        notificationPollEndpointArray[1] = "/api/v1/commerce/history?per_page=10";
        notificationPollTimeoutMsArray[1] = NOTIFICATION_POLL_TIMEOUT;
        notificationDataProcessPageArray[1] = notificationDataProcessPageHistory;
        notificationPollCallbackArray[1] = notificationPollCallbackHistory;
        notificationPollStopPaginatingConditionMetArray[1] = isReturnedDataEmptyHistory;
        notificationPollCaresAboutSinceArray[1] = true;
    }

    ui = new AppUi({
        buttonName: BUTTON_NAME,
        sortOrder: 10,
        home: WALLET_QML_SOURCE,
        additionalAppScreens: SENDASSET_QML_SOURCE,
        onOpened: walletOpened,
        onClosed: walletClosed,
        onMessage: fromQml,
        notificationPollEndpoint: notificationPollEndpointArray,
        notificationPollTimeoutMs: notificationPollTimeoutMsArray,
        notificationDataProcessPage: notificationDataProcessPageArray,
        notificationPollCallback: notificationPollCallbackArray,
        notificationPollStopPaginatingConditionMet: notificationPollStopPaginatingConditionMetArray,
        notificationPollCaresAboutSince: notificationPollCaresAboutSinceArray
    });
    GlobalServices.myUsernameChanged.connect(onUsernameChanged);
    installMarketplaceItemTester();
}

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
    GlobalServices.myUsernameChanged.disconnect(onUsernameChanged);
    deleteSendMoneyParticleEffect();
    uninstallMarketplaceItemTester();
    off();
}

//
// Run the functions.
//
startup();
Script.scriptEnding.connect(shutdown);
}()); // END LOCAL_SCOPE
