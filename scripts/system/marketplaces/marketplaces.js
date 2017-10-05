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

(function () { // BEGIN LOCAL_SCOPE

    Script.include("../libraries/WebTablet.js");

    var MARKETPLACE_URL = "https://metaverse.highfidelity.com/marketplace";
    var MARKETPLACE_URL_INITIAL = MARKETPLACE_URL + "?";  // Append "?" to signal injected script that it's the initial page.
    var MARKETPLACES_URL = Script.resolvePath("../html/marketplaces.html");
    var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");
    var MARKETPLACE_CHECKOUT_QML_PATH_BASE = "qml/hifi/commerce/checkout/Checkout.qml";
    var MARKETPLACE_CHECKOUT_QML_PATH = Script.resourcesPath() + MARKETPLACE_CHECKOUT_QML_PATH_BASE;
    var MARKETPLACE_PURCHASES_QML_PATH = Script.resourcesPath() + "qml/hifi/commerce/purchases/Purchases.qml";
    var MARKETPLACE_WALLET_QML_PATH = Script.resourcesPath() + "qml/hifi/commerce/wallet/Wallet.qml";
    var MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH = "commerce/inspectionCertificate/InspectionCertificate.qml";

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
                },
                canRezCertifiedItems: Entities.canRezCertified || Entities.canRezTmpCertified
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
            var entity = HMD.tabletID;
            Entities.editEntity(entity, { textures: JSON.stringify({ "tex.close": HOME_BUTTON_TEXTURE }) });
            showMarketplace();
        }
    }

    var referrerURL; // Used for updating Purchases QML
    var filterText; // Used for updating Purchases QML
    function onScreenChanged(type, url) {
        onMarketplaceScreen = type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1;
        onCommerceScreen = type === "QML" && (url.indexOf(MARKETPLACE_CHECKOUT_QML_PATH_BASE) !== -1 || url === MARKETPLACE_PURCHASES_QML_PATH || url.indexOf(MARKETPLACE_INSPECTIONCERTIFICATE_QML_PATH) !== -1);
        wireEventBridge(onCommerceScreen);

        if (url === MARKETPLACE_PURCHASES_QML_PATH) {
            tablet.sendToQml({
                method: 'updatePurchases',
                canRezCertifiedItems: Entities.canRezCertified || Entities.canRezTmpCertified,
                referrerURL: referrerURL,
                filterText: filterText
            });
        }

        // for toolbar mode: change button to active when window is first openend, false otherwise.
        marketplaceButton.editProperties({ isActive: onMarketplaceScreen || onCommerceScreen });
        if (type === "Web" && url.indexOf(MARKETPLACE_URL) !== -1) {
            ContextOverlay.isInMarketplaceInspectionMode = true;
        } else {
            ContextOverlay.isInMarketplaceInspectionMode = false;
        }
    }

    function setCertificateInfo(currentEntityWithContextOverlay, itemMarketplaceId) {
        wireEventBridge(true);
        tablet.sendToQml({
            method: 'inspectionCertificate_setMarketplaceId',
            marketplaceId: itemMarketplaceId || Entities.getEntityProperties(currentEntityWithContextOverlay, ['marketplaceID']).marketplaceID
        });
        // ZRF FIXME! Make a call to the endpoint to get item info instead of this silliness
        Script.setTimeout(function () {
            var randomNumber = Math.floor((Math.random() * 150) + 1);
            tablet.sendToQml({
                method: 'inspectionCertificate_setItemInfo',
                itemName: "The Greatest Item",
                itemOwner: "ABCDEFG1234567",
                itemEdition: (Math.floor(Math.random() * randomNumber) + " / " + randomNumber)
            });
        }, 500);
    }

    function onUsernameChanged() {
        if (onMarketplaceScreen) {
            tablet.gotoWebScreen(MARKETPLACE_URL_INITIAL, MARKETPLACES_INJECT_SCRIPT_URL);
        }
    }

    marketplaceButton.clicked.connect(onClick);
    tablet.screenChanged.connect(onScreenChanged);
    Entities.canWriteAssetsChanged.connect(onCanWriteAssetsChanged);
    ContextOverlay.contextOverlayClicked.connect(setCertificateInfo);
    GlobalServices.myUsernameChanged.connect(onUsernameChanged);

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
                    params: parsedJsonMessage,
                    canRezCertifiedItems: Entities.canRezCertified || Entities.canRezTmpCertified
                });
            } else if (parsedJsonMessage.type === "REQUEST_SETTING") {
                tablet.emitScriptEvent(JSON.stringify({
                    type: "marketplaces",
                    action: "commerceSetting",
                    data: {
                        commerceMode: Settings.getValue("commerce", false),
                        userIsLoggedIn: Account.loggedIn,
                        walletNeedsSetup: Wallet.walletStatus === 1
                    }
                }));
            } else if (parsedJsonMessage.type === "PURCHASES") {
                referrerURL = parsedJsonMessage.referrerURL;
                filterText = "";
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
            } else if (parsedJsonMessage.type === "LOGIN") {
                openLoginWindow();
            } else if (parsedJsonMessage.type === "WALLET_SETUP") {
                tablet.pushOntoStack(MARKETPLACE_WALLET_QML_PATH);
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
                tablet.pushOntoStack(MARKETPLACE_WALLET_QML_PATH);
                break;
            case 'purchases_walletNotSetUp':
            case 'checkout_walletNotSetUp':
                wireEventBridge(true);
                tablet.sendToQml({
                    method: 'updateWalletReferrer',
                    referrer: "purchases"
                });
                tablet.pushOntoStack(MARKETPLACE_WALLET_QML_PATH);
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
            case 'checkout_continueShopping':
                tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.itemId, MARKETPLACES_INJECT_SCRIPT_URL);
                //tablet.popFromStack();
                break;
            case 'purchases_itemInfoClicked':
                var itemId = message.itemId;
                if (itemId && itemId !== "") {
                    tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + itemId, MARKETPLACES_INJECT_SCRIPT_URL);
                }
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
                tablet.loadQMLSource("TabletAddressDialog.qml");
                break;
            case 'purchases_itemCertificateClicked':
                setCertificateInfo("", message.itemMarketplaceId);
                break;
            case 'inspectionCertificate_closeClicked':
                tablet.gotoHomeScreen();
                break;
            case 'inspectionCertificate_showInMarketplaceClicked':
                tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.itemId, MARKETPLACES_INJECT_SCRIPT_URL);
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
            default:
                print('Unrecognized message from Checkout.qml or Purchases.qml: ' + JSON.stringify(message));
        }
    }

}()); // END LOCAL_SCOPE
