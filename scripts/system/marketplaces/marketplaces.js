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
   DesktopPreviewProvider, ResourceRequestObserver */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

var selectionDisplay = null; // for gridTool.js to ignore

(function () { // BEGIN LOCAL_SCOPE
var AppUi = Script.require('appUi');
Script.include("/~/system/libraries/gridTool.js");
Script.include("/~/system/libraries/connectionUtils.js");

var MARKETPLACE_CHECKOUT_QML_PATH = "hifi/commerce/checkout/Checkout.qml";
var MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH = "hifi/commerce/inspectionCertificate/InspectionCertificate.qml";
var MARKETPLACE_ITEM_TESTER_QML_PATH = "hifi/commerce/marketplaceItemTester/MarketplaceItemTester.qml";
var MARKETPLACE_PURCHASES_QML_PATH = "hifi/commerce/wallet/Wallet.qml"; // HRS FIXME "hifi/commerce/purchases/Purchases.qml";
var MARKETPLACE_WALLET_QML_PATH = "hifi/commerce/wallet/Wallet.qml";
var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");
var MARKETPLACES_URL = Script.resolvePath("../html/marketplaces.html");
var METAVERSE_SERVER_URL = Account.metaverseServerURL;
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


var resourceRequestEvents = [];
function signalResourceRequestEvent(data) {
    // Once we can tie resource request events to specific resources,
    // we will have to update the "0" in here.
    var resourceData = "from: " + data.extra + ": " + data.url.toString().replace("__NONE__,", "");

    if (resourceObjectsInTest[0].resourceDataArray.indexOf(resourceData) === -1) {
        resourceObjectsInTest[0].resourceDataArray.push(resourceData);

        resourceObjectsInTest[0].resourceAccessEventText += "[" + data.date.toISOString() + "] " +
            resourceData + "\n";

        ui.tablet.sendToQml({
            method: "resourceRequestEvent",
            data: data,
            resourceAccessEventText: resourceObjectsInTest[0].resourceAccessEventText
        });
    }
}

function onResourceRequestEvent(data) {
    // Once we can tie resource request events to specific resources,
    // we will have to update the "0" in here.
    if (resourceObjectsInTest[0] && resourceObjectsInTest[0].currentlyRecordingResources) {
        var resourceRequestEvent = {
            "date": new Date(),
            "url": data.url,
            "callerId": data.callerId,
            "extra": data.extra
        };
        resourceRequestEvents.push(resourceRequestEvent);
        signalResourceRequestEvent(resourceRequestEvent);
    }
}

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

    Overlays.editOverlay(HMD.tabletID, { isVisibleInSecondaryCamera: visibleInSecondaryCam });
    Overlays.editOverlay(HMD.homeButtonID, { isVisibleInSecondaryCamera: visibleInSecondaryCam });
    Overlays.editOverlay(HMD.homeButtonHighlightID, { isVisibleInSecondaryCamera: visibleInSecondaryCam });
    Overlays.editOverlay(HMD.tabletScreenID, { isVisibleInSecondaryCamera: visibleInSecondaryCam });
}

function openWallet() {
    ui.open(MARKETPLACE_WALLET_QML_PATH);
}
function setupWallet(referrer) {
    // Needs to be done within the QML page in order to get access to QmlCommerce
    openWallet();
    var ALLOWANCE_FOR_EVENT_BRIDGE_SETUP = 0;
    Script.setTimeout(function () {
        ui.tablet.sendToQml({
            method: 'updateWalletReferrer',
            referrer: referrer
        });
    }, ALLOWANCE_FOR_EVENT_BRIDGE_SETUP);
}

function onMarketplaceOpen(referrer) {
    var match;
    if (Account.loggedIn && walletNeedsSetup()) {
        if (referrer === MARKETPLACE_URL_INITIAL) {
            setupWallet('marketplace cta');
        } else {
            match = referrer.match(/\/item\/(\w+)$/);
            if (match && match[1]) {
                setupWallet(match[1]);
            } else if (referrer.indexOf(METAVERSE_SERVER_URL) === -1) { // not a url
                setupWallet(referrer);
            } else {
                print("WARNING: opening marketplace to", referrer, "without wallet setup.");
            }
        }
    }
}

function openMarketplace(optionalItemOrUrl) {
    // This is a bit of a kluge, but so is the whole file.
    // If given a whole path, use it with no cta.
    // If given an id, build the appropriate url and use the id as the cta.
    // Otherwise, use home and 'marketplace cta'.
    // AND... if call onMarketplaceOpen to setupWallet if we need to.
    var url = optionalItemOrUrl || MARKETPLACE_URL_INITIAL;
    // If optionalItemOrUrl contains the metaverse base, then it's a url, not an item id.
    if (optionalItemOrUrl && optionalItemOrUrl.indexOf(METAVERSE_SERVER_URL) === -1) {
        url = MARKETPLACE_URL + '/items/' + optionalItemOrUrl;
    }
    ui.open(url, MARKETPLACES_INJECT_SCRIPT_URL);
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
        openMarketplace();
    }
}

function walletNeedsSetup() {
    return WalletScriptingInterface.walletStatus === 1;
}

function sendCommerceSettings() {
    ui.sendToHtml({
        type: "marketplaces",
        action: "commerceSetting",
        data: {
            commerceMode: Settings.getValue("commerce", true),
            userIsLoggedIn: Account.loggedIn,
            walletNeedsSetup: walletNeedsSetup(),
            metaverseServerURL: Account.metaverseServerURL,
            limitedCommerce: WalletScriptingInterface.limitedCommerce
        }
    });
}

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

function defaultFor(arg, val) {
    return typeof arg !== 'undefined' ? arg : val;
}

var CERT_ID_URLPARAM_LENGTH = 15; // length of "certificate_id="
function rezEntity(itemHref, itemType, marketplaceItemTesterId) {
    var isWearable = itemType === "wearable";
    var success = Clipboard.importEntities(itemHref, true, marketplaceItemTesterId);
    var wearableLocalPosition = null;
    var wearableLocalRotation = null;
    var wearableLocalDimensions = null;
    var wearableDimensions = null;
    marketplaceItemTesterId = defaultFor(marketplaceItemTesterId, -1);

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
            certPos += CERT_ID_URLPARAM_LENGTH;
            var certURLEncoded = itemHref.substring(certPos);
            var certB64Encoded = decodeURIComponent(certURLEncoded);
            for (var key in wearableTransforms) {
                if (wearableTransforms.hasOwnProperty(key)) {
                    var certificateTransforms = wearableTransforms[key].certificateTransforms;
                    if (certificateTransforms) {
                        for (var certID in certificateTransforms) {
                            if (certificateTransforms.hasOwnProperty(certID) &&
                                certID === certB64Encoded) {
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
                    // Distance to move entities parallel to targetDirection.
                    var deltaParallel = HALF_TREE_SCALE;
                    // Distance to move entities perpendicular to targetDirection.
                    var deltaPerpendicular = Vec3.ZERO;
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
                    for (var editEntityIndex = 0,
                        numEntities = pastedEntityIDs.length; editEntityIndex < numEntities; editEntityIndex++) {
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
        // This is the chooser between marketplaces. Only OUR markteplace
        // requires/makes-use-of wallet, so doesn't go through openMarketplace bottleneck.
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
        setupWallet('marketplace cta');
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

var resourceObjectsInTest = [];
function signalNewResourceObjectInTest(resourceObject) {
    ui.tablet.sendToQml({
        method: "newResourceObjectInTest",
        resourceObject: resourceObject
    });
}

var onQmlMessageReceived = function onQmlMessageReceived(message) {
    if (message.messageSrc === "HTML") {
        return;
    }
    switch (message.method) {
    case 'gotoBank':
        ui.close();
        if (Account.metaverseServerURL.indexOf("staging") >= 0) {
            Window.location = "hifi://hifiqa-master-metaverse-staging"; // So that we can test in staging.
        } else {
            Window.location = "hifi://BankOfHighFidelity";
        }
        break;
    case 'checkout_openRecentActivity':
        ui.open(MARKETPLACE_WALLET_QML_PATH);
        wireQmlEventBridge(true);
        ui.tablet.sendToQml({
            method: 'checkout_openRecentActivity'
        });
        break;
    case 'checkout_setUpClicked':
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
        openMarketplace(message.params);
        break;
    case 'header_goToPurchases':
    case 'checkout_goToPurchases':
        referrerURL = MARKETPLACE_URL_INITIAL;
        filterText = message.filterText;
        ui.open(MARKETPLACE_PURCHASES_QML_PATH);
        break;
    case 'checkout_itemLinkClicked':
        openMarketplace(message.itemId);
        break;
    case 'checkout_continue':
        openMarketplace();
        break;
    case 'checkout_rezClicked':
    case 'purchases_rezClicked':
    case 'tester_rezClicked':
        rezEntity(message.itemHref, message.itemType, message.itemId);
        break;
    case 'tester_newResourceObject':
        var resourceObject = message.resourceObject;
        resourceObjectsInTest = []; // REMOVE THIS once we support specific referrers
        resourceObject.currentlyRecordingResources = false;
        resourceObject.resourceAccessEventText = "";
        resourceObjectsInTest[resourceObject.resourceObjectId] = resourceObject;
        resourceObjectsInTest[resourceObject.resourceObjectId].resourceDataArray = [];
        signalNewResourceObjectInTest(resourceObject);
        break;
    case 'tester_updateResourceObjectAssetType':
        resourceObjectsInTest[message.objectId].assetType = message.assetType;
        break;
    case 'tester_deleteResourceObject':
        delete resourceObjectsInTest[message.objectId];
        break;
    case 'tester_updateResourceRecordingStatus':
        resourceObjectsInTest[message.objectId].currentlyRecordingResources = message.status;
        if (message.status) {
            resourceObjectsInTest[message.objectId].resourceDataArray = [];
            resourceObjectsInTest[message.objectId].resourceAccessEventText = "";
        }
        break;
    case 'header_marketplaceImageClicked':
        openMarketplace(message.referrerURL);
        break;
    case 'purchases_goToMarketplaceClicked':
        openMarketplace();
        break;
    case 'updateItemClicked':
        openMarketplace(message.upgradeUrl + "?edition=" + message.itemEdition);
        break;
    case 'passphrasePopup_cancelClicked':
    case 'needsLogIn_cancelClicked':
        // Should/must NOT check for wallet setup.
        ui.open(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
        break;
    case 'needsLogIn_loginClicked':
        openLoginWindow();
        break;
    case 'disableHmdPreview':
        if (!savedDisablePreviewOption) {
            DesktopPreviewProvider.setPreviewDisabledReason("SECURE_SCREEN");
            Menu.setIsOptionChecked("Disable Preview", true);
            setTabletVisibleInSecondaryCamera(false);
        }
        break;
    case 'maybeEnableHmdPreview':
        maybeEnableHMDPreview();
        break;
    case 'checkout_openGoTo':
        ui.open("hifi/tablet/TabletAddressDialog.qml");
        break;
    case 'inspectionCertificate_closeClicked':
        ui.close();
        break;
    case 'inspectionCertificate_requestOwnershipVerification':
        ContextOverlay.requestOwnershipVerification(message.entity);
        break;
    case 'inspectionCertificate_showInMarketplaceClicked':
        openMarketplace(message.marketplaceUrl);
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
    case 'http.request':
        // Handled elsewhere, don't log.
        break;
    // All of these are handled by wallet.js
    case 'purchases_updateWearables':
    case 'enable_ChooseRecipientNearbyMode':
    case 'disable_ChooseRecipientNearbyMode':
    case 'sendAsset_sendPublicly':
    case 'refreshConnections':
    case 'transactionHistory_goToBank':
    case 'purchases_walletNotSetUp':
    case 'purchases_openGoTo':
    case 'purchases_itemInfoClicked':
    case 'purchases_itemCertificateClicked':
        case 'clearShouldShowDotHistory':
        case 'giftAsset':
        break;
    default:
        print('marketplaces.js: Unrecognized message from Checkout.qml');
    }
};

function pushResourceObjectsInTest() {
    var maxResourceObjectId = -1;
    var length = resourceObjectsInTest.length;
    for (var i = 0; i < length; i++) {
        if (i in resourceObjectsInTest) {
            signalNewResourceObjectInTest(resourceObjectsInTest[i]);
            var resourceObjectId = resourceObjectsInTest[i].resourceObjectId;
            maxResourceObjectId = (maxResourceObjectId < resourceObjectId) ? parseInt(resourceObjectId) : maxResourceObjectId;
        }
    }
    // N.B. Thinking about removing the following sendToQml? Be sure
    // that the marketplace item tester QML has heard from us, at least
    // so that it can indicate to the user that all of the resoruce
    // objects in test have been transmitted to it.
    //ui.tablet.sendToQml({ method: "nextObjectIdInTest", id: maxResourceObjectId + 1 });
    // Since, for now, we only support 1 object in test, always send id: 0
    ui.tablet.sendToQml({ method: "nextObjectIdInTest", id: 0 });
}

// Function Name: onTabletScreenChanged()
//
// Description:
//   -Called when the TabletScriptingInterface::screenChanged() signal is emitted. The "type" argument can be either the string
//    value of "Home", "Web", "Menu", "QML", or "Closed". The "url" argument is only valid for Web and QML.

var onCommerceScreen = false;
var onInspectionCertificateScreen = false;
var onMarketplaceItemTesterScreen = false;
var onMarketplaceScreen = false;
var onWalletScreen = false;
var onTabletScreenChanged = function onTabletScreenChanged(type, url) {
    ui.setCurrentVisibleScreenMetadata(type, url);
    onMarketplaceScreen = type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1;
    onInspectionCertificateScreen = type === "QML" && url.indexOf(MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH) !== -1;
    var onWalletScreenNow = url.indexOf(MARKETPLACE_WALLET_QML_PATH) !== -1;
    var onCommerceScreenNow = type === "QML" && (
        url.indexOf(MARKETPLACE_CHECKOUT_QML_PATH) !== -1 ||
        url === MARKETPLACE_PURCHASES_QML_PATH ||
        url.indexOf(MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH) !== -1);
    var onMarketplaceItemTesterScreenNow = (
        url.indexOf(MARKETPLACE_ITEM_TESTER_QML_PATH) !== -1 ||
        url === MARKETPLACE_ITEM_TESTER_QML_PATH);

    if ((!onWalletScreenNow && onWalletScreen) ||
        (!onCommerceScreenNow && onCommerceScreen) ||
        (!onMarketplaceItemTesterScreenNow && onMarketplaceScreen)
    ) {
        // exiting wallet, commerce, or marketplace item tester screen
        maybeEnableHMDPreview();
    }

    onCommerceScreen = onCommerceScreenNow;
    onWalletScreen = onWalletScreenNow;
    onMarketplaceItemTesterScreen = onMarketplaceItemTesterScreenNow;
    wireQmlEventBridge(onCommerceScreen || onWalletScreen || onMarketplaceItemTesterScreen);

    if (url === MARKETPLACE_PURCHASES_QML_PATH) {
        // FIXME? Is there a race condition here in which the event
        // bridge may not be up yet? If so, Script.setTimeout(..., 750)
        // may help avoid the condition.
        ui.tablet.sendToQml({
            method: 'updatePurchases',
            referrerURL: referrerURL,
            filterText: filterText
        });
        referrerURL = "";
        filterText = "";
    }

    var wasIsOpen = ui.isOpen;
    ui.isOpen = (onMarketplaceScreen || onCommerceScreen) && !onWalletScreen;
    ui.buttonActive(ui.isOpen);

    if (wasIsOpen !== ui.isOpen && Keyboard.raised) {
        Keyboard.raised = false;
    }

    if (type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1) {
        ContextOverlay.isInMarketplaceInspectionMode = true;
    } else {
        ContextOverlay.isInMarketplaceInspectionMode = false;
    }

    if (onInspectionCertificateScreen) {
        setCertificateInfo(contextOverlayEntity);
    }

    if (onCommerceScreen) {
        WalletScriptingInterface.refreshWalletStatus();
    } else {
        if (onMarketplaceScreen) {
            onMarketplaceOpen('marketplace cta');
        }
        ui.tablet.sendToQml({
            method: 'inspectionCertificate_resetCert'
        });
        off();
    }

    if (onMarketplaceItemTesterScreen) {
        // Why setTimeout? The QML event bridge, wired above, requires a
        // variable amount of time to come up, in practice less than
        // 750ms.
        Script.setTimeout(pushResourceObjectsInTest, 750);
    }

    console.debug(ui.buttonName + " app reports: Tablet screen changed.\nNew screen type: " + type +
        "\nNew screen URL: " + url + "\nCurrent app open status: " + ui.isOpen + "\n");
};


var BUTTON_NAME = "MARKET";
var MARKETPLACE_URL = METAVERSE_SERVER_URL + "/marketplace";
// Append "?" if necessary to signal injected script that it's the initial page.
var MARKETPLACE_URL_INITIAL = MARKETPLACE_URL + (MARKETPLACE_URL.indexOf("?") > -1 ? "" : "?");
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
    WalletScriptingInterface.walletStatusChanged.connect(sendCommerceSettings);
    Window.messageBoxClosed.connect(onMessageBoxClosed);
    ResourceRequestObserver.resourceRequestEvent.connect(onResourceRequestEvent);

    WalletScriptingInterface.refreshWalletStatus();
}

function off() {
}
function shutdown() {
    maybeEnableHMDPreview();

    Window.messageBoxClosed.disconnect(onMessageBoxClosed);
    WalletScriptingInterface.walletStatusChanged.disconnect(sendCommerceSettings);
    ui.tablet.webEventReceived.disconnect(onWebEventReceived);
    GlobalServices.myUsernameChanged.disconnect(onUsernameChanged);
    Entities.canWriteAssetsChanged.disconnect(onCanWriteAssetsChanged);
    ContextOverlay.contextOverlayClicked.disconnect(openInspectionCertificateQML);
    ResourceRequestObserver.resourceRequestEvent.disconnect(onResourceRequestEvent);

    off();
}

//
// Run the functions.
//
startup();
Script.scriptEnding.connect(shutdown);
}()); // END LOCAL_SCOPE
