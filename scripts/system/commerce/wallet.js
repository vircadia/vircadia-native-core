"use strict";
/*jslint vars:true, plusplus:true, forin:true*/
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

/*global XXX */

(function () { // BEGIN LOCAL_SCOPE
    Script.include("/~/system/libraries/accountUtils.js");

    var MARKETPLACE_URL = Account.metaverseServerURL + "/marketplace";

    // Function Name: onButtonClicked()
    //
    // Description:
    //   -Fired when the app button is pressed.
    //
    // Relevant Variables:
    //   -WALLET_QML_SOURCE: The path to the Wallet QML
    //   -onWalletScreen: true/false depending on whether we're looking at the app.
    var WALLET_QML_SOURCE = "hifi/commerce/wallet/Wallet.qml";
    var MARKETPLACE_PURCHASES_QML_PATH = "hifi/commerce/purchases/Purchases.qml";
    var onWalletScreen = false;
    function onButtonClicked() {
        if (!tablet) {
            print("Warning in buttonClicked(): 'tablet' undefined!");
            return;
        }
        if (onWalletScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource(WALLET_QML_SOURCE);
        }
    }

    // Function Name: sendToQml()
    //
    // Description:
    //   -Use this function to send a message to the QML (i.e. to change appearances). The "message" argument is what is sent to
    //    the QML in the format "{method, params}", like json-rpc. See also fromQml().
    function sendToQml(message) {
        tablet.sendToQml(message);
    }

    // Function Name: fromQml()
    //
    // Description:
    //   -Called when a message is received from SpectatorCamera.qml. The "message" argument is what is sent from the QML
    //    in the format "{method, params}", like json-rpc. See also sendToQml().
    var isHmdPreviewDisabled = true;
    var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");
    function fromQml(message) {
        switch (message.method) {
            case 'passphrasePopup_cancelClicked':
            case 'needsLogIn_cancelClicked':
                tablet.gotoHomeScreen();
                break;
            case 'walletSetup_cancelClicked':
                switch (message.referrer) {
                    case '': // User clicked "Wallet" app
                    case undefined:
                    case null:
                        tablet.gotoHomeScreen();
                        break;
                    case 'purchases':
                    case 'marketplace cta':
                    case 'mainPage':
                        tablet.gotoWebScreen(MARKETPLACE_URL, MARKETPLACES_INJECT_SCRIPT_URL);
                        break;
                    default: // User needs to return to an individual marketplace item URL
                        tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.referrer, MARKETPLACES_INJECT_SCRIPT_URL);
                        break;
                }
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
            case 'passphraseReset':
                onButtonClicked();
                onButtonClicked();
                break;
            case 'walletReset':
                Settings.setValue("isFirstUseOfPurchases", true);
                onButtonClicked();
                onButtonClicked();
                break;
            case 'transactionHistory_linkClicked':
                tablet.gotoWebScreen(message.marketplaceLink, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'goToPurchases':
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
                break;
            case 'goToMarketplaceItemPage':
                tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.itemId, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            default:
                print('Unrecognized message from QML:', JSON.stringify(message));
        }
    }

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

    // Function Name: onTabletScreenChanged()
    //
    // Description:
    //   -Called when the TabletScriptingInterface::screenChanged() signal is emitted. The "type" argument can be either the string
    //    value of "Home", "Web", "Menu", "QML", or "Closed". The "url" argument is only valid for Web and QML.
    function onTabletScreenChanged(type, url) {
        onWalletScreen = (type === "QML" && url === WALLET_QML_SOURCE);
        wireEventBridge(onWalletScreen);
        // Change button to active when window is first openend, false otherwise.
        if (button) {
            button.editProperties({ isActive: onWalletScreen });
        }
    }

    //
    // Manage the connection between the button and the window.
    //
    var button;
    var buttonName = "WALLET";
    var tablet = null;
    var walletEnabled = Settings.getValue("commerce", true);
    function startup() {
        if (walletEnabled) {
            tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            button = tablet.addButton({
                text: buttonName,
                icon: "icons/tablet-icons/wallet-i.svg",
                activeIcon: "icons/tablet-icons/wallet-a.svg",
                sortOrder: 10
            });
            button.clicked.connect(onButtonClicked);
            tablet.screenChanged.connect(onTabletScreenChanged);
        }
    }
    function shutdown() {
        button.clicked.disconnect(onButtonClicked);
        tablet.removeButton(button);
        if (tablet) {
            tablet.screenChanged.disconnect(onTabletScreenChanged);
            if (onWalletScreen) {
                tablet.gotoHomeScreen();
            }
        }
    }

    //
    // Run the functions.
    //
    startup();
    Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
