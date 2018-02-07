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
   Quat, MyAvatar, Clipboard, Menu, Grid, Uuid, GlobalServices, openLoginWindow */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

var selectionDisplay = null; // for gridTool.js to ignore

(function () { // BEGIN LOCAL_SCOPE

    Script.include("/~/system/libraries/WebTablet.js");
    Script.include("/~/system/libraries/gridTool.js");

    var METAVERSE_SERVER_URL = Account.metaverseServerURL;
    var MARKETPLACE_URL = METAVERSE_SERVER_URL + "/marketplace";
    var MARKETPLACE_URL_INITIAL = MARKETPLACE_URL + "?";  // Append "?" to signal injected script that it's the initial page.
    var MARKETPLACES_URL = Script.resolvePath("../html/marketplaces.html");
    var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");
    var MARKETPLACE_CHECKOUT_QML_PATH = "hifi/commerce/checkout/Checkout.qml";
    var MARKETPLACE_PURCHASES_QML_PATH = "hifi/commerce/purchases/Purchases.qml";
    var MARKETPLACE_WALLET_QML_PATH = "hifi/commerce/wallet/Wallet.qml";
    var MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH = "commerce/inspectionCertificate/InspectionCertificate.qml";
    var REZZING_SOUND = SoundCache.getSound(Script.resolvePath("../assets/sounds/rezzing.wav"));

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

    var onMarketplaceScreen = false;
    var onCommerceScreen = false;

    var debugCheckout = false;
    var debugError = false;
    function showMarketplace() {
        if (!debugCheckout) {
            UserActivityLogger.openedMarketplace();
            tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
        } else {
            tablet.pushOntoStack(MARKETPLACE_CHECKOUT_QML_PATH);
            tablet.sendToQml({
                method: 'updateCheckoutQML', params: {
                    itemId: '0d90d21c-ce7a-4990-ad18-e9d2cf991027',
                    itemName: 'Test Flaregun',
                    itemPrice: (debugError ? 10 : 17),
                    itemHref: 'http://mpassets.highfidelity.com/0d90d21c-ce7a-4990-ad18-e9d2cf991027-v1/flaregun.json',
                    categories: ["Wearables", "Miscellaneous"]
                }
            });
        }
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var marketplaceButton = tablet.addButton({
        icon: "icons/tablet-icons/market-i.svg",
        activeIcon: "icons/tablet-icons/market-a.svg",
        text: "MARKET",
        sortOrder: 9
    });

    function onCanWriteAssetsChanged() {
        var message = CAN_WRITE_ASSETS + " " + Entities.canWriteAssets();
        tablet.emitScriptEvent(message);
    }

    function onClick() {
        if (onMarketplaceScreen || onCommerceScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            Wallet.refreshWalletStatus();
            var entity = HMD.tabletID;
            Entities.editEntity(entity, { textures: JSON.stringify({ "tex.close": HOME_BUTTON_TEXTURE }) });
            showMarketplace();
        }
    }

    var referrerURL; // Used for updating Purchases QML
    var filterText; // Used for updating Purchases QML
    function onScreenChanged(type, url) {
        onMarketplaceScreen = type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1;
        onWalletScreen = url.indexOf(MARKETPLACE_WALLET_QML_PATH) !== -1;
        onCommerceScreen = type === "QML" && (url.indexOf(MARKETPLACE_CHECKOUT_QML_PATH) !== -1 || url === MARKETPLACE_PURCHASES_QML_PATH
            || url.indexOf(MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH) !== -1);
        wireEventBridge(onMarketplaceScreen || onCommerceScreen || onWalletScreen);

        if (url === MARKETPLACE_PURCHASES_QML_PATH) {
            tablet.sendToQml({
                method: 'updatePurchases',
                referrerURL: referrerURL,
                filterText: filterText
            });
        }

        // for toolbar mode: change button to active when window is first openend, false otherwise.
        marketplaceButton.editProperties({ isActive: (onMarketplaceScreen || onCommerceScreen) && !onWalletScreen });
        if (type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1) {
            ContextOverlay.isInMarketplaceInspectionMode = true;
        } else {
            ContextOverlay.isInMarketplaceInspectionMode = false;
        }

        if (!onCommerceScreen) {
            tablet.sendToQml({
                method: 'inspectionCertificate_resetCert'
            });
        }
    }

    function openWallet() {
        tablet.pushOntoStack(MARKETPLACE_WALLET_QML_PATH);
    }

    function setCertificateInfo(currentEntityWithContextOverlay, itemCertificateId) {
        wireEventBridge(true);
        var certificateId = itemCertificateId || (Entities.getEntityProperties(currentEntityWithContextOverlay, ['certificateID']).certificateID);
        tablet.sendToQml({
            method: 'inspectionCertificate_setCertificateId',
            certificateId: certificateId
        });
    }

    function onUsernameChanged() {
        if (onMarketplaceScreen) {
            tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
        }
    }

    function sendCommerceSettings() {
        tablet.emitScriptEvent(JSON.stringify({
            type: "marketplaces",
            action: "commerceSetting",
            data: {
                commerceMode: Settings.getValue("commerce", true),
                userIsLoggedIn: Account.loggedIn,
                walletNeedsSetup: Wallet.walletStatus === 1,
                metaverseServerURL: Account.metaverseServerURL
            }
        }));
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
            { x: 1, y: 1, z: 1 },
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

    function rezEntity(itemHref, isWearable) {
        var success = Clipboard.importEntities(itemHref);
        var wearableLocalPosition = null;
        var wearableLocalRotation = null;
        var wearableLocalDimensions = null;
        var wearableDimensions = null;

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

    marketplaceButton.clicked.connect(onClick);
    tablet.screenChanged.connect(onScreenChanged);
    Entities.canWriteAssetsChanged.connect(onCanWriteAssetsChanged);
    ContextOverlay.contextOverlayClicked.connect(setCertificateInfo);
    GlobalServices.myUsernameChanged.connect(onUsernameChanged);
    Wallet.walletStatusChanged.connect(sendCommerceSettings);
    Wallet.refreshWalletStatus();

    function onMessage(message) {

        if (message === GOTO_DIRECTORY) {
            tablet.gotoWebScreen(MARKETPLACES_URL, MARKETPLACES_INJECT_SCRIPT_URL);
        } else if (message === QUERY_CAN_WRITE_ASSETS) {
            tablet.emitScriptEvent(CAN_WRITE_ASSETS + " " + Entities.canWriteAssets());
        } else if (message === WARN_USER_NO_PERMISSIONS) {
            Window.alert(NO_PERMISSIONS_ERROR_MESSAGE);
        } else if (message.slice(0, CLARA_IO_STATUS.length) === CLARA_IO_STATUS) {
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
        } else if (message.slice(0, CLARA_IO_DOWNLOAD.length) === CLARA_IO_DOWNLOAD) {
            if (messageBox !== null) {
                Window.closeMessageBox(messageBox);
                messageBox = null;
            }
            return;
        } else if (message === CLARA_IO_CANCELLED_DOWNLOAD) {
            isDownloadBeingCancelled = false;
        } else {
            var parsedJsonMessage = JSON.parse(message);
            if (parsedJsonMessage.type === "CHECKOUT") {
                wireEventBridge(true);
                tablet.pushOntoStack(MARKETPLACE_CHECKOUT_QML_PATH);
                tablet.sendToQml({
                    method: 'updateCheckoutQML',
                    params: parsedJsonMessage
                });
            } else if (parsedJsonMessage.type === "REQUEST_SETTING") {
                sendCommerceSettings();
            } else if (parsedJsonMessage.type === "PURCHASES") {
                referrerURL = parsedJsonMessage.referrerURL;
                filterText = "";
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
            } else if (parsedJsonMessage.type === "LOGIN") {
                openLoginWindow();
            } else if (parsedJsonMessage.type === "WALLET_SETUP") {
                wireEventBridge(true);
                tablet.sendToQml({
                    method: 'updateWalletReferrer',
                    referrer: "marketplace cta"
                });
                openWallet();
            } else if (parsedJsonMessage.type === "MY_ITEMS") {
                referrerURL = MARKETPLACE_URL_INITIAL;
                filterText = "";
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
                wireEventBridge(true);
                tablet.sendToQml({
                    method: 'purchases_showMyItems'
                });
            }
        }
    }

    tablet.webEventReceived.connect(onMessage);

    Script.scriptEnding.connect(function () {
        if (onMarketplaceScreen || onCommerceScreen) {
            tablet.gotoHomeScreen();
        }
        tablet.removeButton(marketplaceButton);
        tablet.screenChanged.disconnect(onScreenChanged);
        ContextOverlay.contextOverlayClicked.disconnect(setCertificateInfo);
        tablet.webEventReceived.disconnect(onMessage);
        Entities.canWriteAssetsChanged.disconnect(onCanWriteAssetsChanged);
        GlobalServices.myUsernameChanged.disconnect(onUsernameChanged);
        Wallet.walletStatusChanged.disconnect(sendCommerceSettings);
    });



    // Function Name: wireEventBridge()
    //
    // Description:
    //   -Used to connect/disconnect the script's response to the tablet's "fromQml" signal. Set the "on" argument to enable or
    //    disable to event bridge.
    //
    // Relevant Variables:
    //   -hasEventBridge: true/false depending on whether we've already connected the event bridge.
    var hasEventBridge = false;
    function wireEventBridge(on) {
        if (!tablet) {
            print("Warning in wireEventBridge(): 'tablet' undefined!");
            return;
        }
        if (on) {
            if (!hasEventBridge) {
                tablet.fromQml.connect(fromQml);
                hasEventBridge = true;
            }
        } else {
            if (hasEventBridge) {
                tablet.fromQml.disconnect(fromQml);
                hasEventBridge = false;
            }
        }
    }

    // Function Name: fromQml()
    //
    // Description:
    //   -Called when a message is received from Checkout.qml. The "message" argument is what is sent from the Checkout QML
    //    in the format "{method, params}", like json-rpc.
    var isHmdPreviewDisabled = true;
    function fromQml(message) {
        switch (message.method) {
            case 'purchases_openWallet':
            case 'checkout_openWallet':
            case 'checkout_setUpClicked':
                openWallet();
                break;
            case 'purchases_walletNotSetUp':
                wireEventBridge(true);
                tablet.sendToQml({
                    method: 'updateWalletReferrer',
                    referrer: "purchases"
                });
                openWallet();
                break;
            case 'checkout_walletNotSetUp':
                wireEventBridge(true);
                tablet.sendToQml({
                    method: 'updateWalletReferrer',
                    referrer: message.referrer === "itemPage" ? message.itemId : message.referrer
                });
                openWallet();
                break;
            case 'checkout_cancelClicked':
                tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.params, MARKETPLACES_INJECT_SCRIPT_URL);
                // TODO: Make Marketplace a QML app that's a WebView wrapper so we can use the app stack.
                // I don't think this is trivial to do since we also want to inject some JS into the DOM.
                //tablet.popFromStack();
                break;
            case 'header_goToPurchases':
            case 'checkout_goToPurchases':
                referrerURL = MARKETPLACE_URL_INITIAL;
                filterText = message.filterText;
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
                break;
            case 'checkout_itemLinkClicked':
                tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.itemId, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'checkout_continueShopping':
                tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
                //tablet.popFromStack();
                break;
            case 'purchases_itemInfoClicked':
                var itemId = message.itemId;
                if (itemId && itemId !== "") {
                    tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + itemId, MARKETPLACES_INJECT_SCRIPT_URL);
                }
                break;
            case 'checkout_rezClicked':
            case 'purchases_rezClicked':
                rezEntity(message.itemHref, message.isWearable);
                break;
            case 'header_marketplaceImageClicked':
            case 'purchases_backClicked':
                tablet.gotoWebScreen(message.referrerURL, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'purchases_goToMarketplaceClicked':
                tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'passphrasePopup_cancelClicked':
            case 'needsLogIn_cancelClicked':
                tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'needsLogIn_loginClicked':
                openLoginWindow();
                break;
            case 'disableHmdPreview':
                isHmdPreviewDisabled = Menu.isOptionChecked("Disable Preview");
                Menu.setIsOptionChecked("Disable Preview", true);
                break;
            case 'maybeEnableHmdPreview':
                Menu.setIsOptionChecked("Disable Preview", isHmdPreviewDisabled);
                break;
            case 'purchases_openGoTo':
                tablet.loadQMLSource("hifi/tablet/TabletAddressDialog.qml");
                break;
            case 'purchases_itemCertificateClicked':
                setCertificateInfo("", message.itemCertificateId);
                break;
            case 'inspectionCertificate_closeClicked':
                tablet.gotoHomeScreen();
                break;
            case 'inspectionCertificate_showInMarketplaceClicked':
                tablet.gotoWebScreen(message.marketplaceUrl, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'header_myItemsClicked':
                referrerURL = MARKETPLACE_URL_INITIAL;
                filterText = "";
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
                wireEventBridge(true);
                tablet.sendToQml({
                    method: 'purchases_showMyItems'
                });
                break;
            case 'refreshConnections':
            case 'enable_ChooseRecipientNearbyMode':
            case 'disable_ChooseRecipientNearbyMode':
            case 'sendMoney_sendPublicly':
                // NOP
                break;
            default:
                print('Unrecognized message from Checkout.qml or Purchases.qml: ' + JSON.stringify(message));
        }
    }

}()); // END LOCAL_SCOPE
