//
//  Purchases.qml
//  qml/hifi/commerce/purchases
//
//  Purchases
//
//  Created by Zach Fox on 2017-08-25
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls
import "../wallet" as HifiWallet

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "initialize";
    property string referrerURL: "";
    property bool securityImageResultReceived: false;
    property bool purchasesReceived: false;
    property bool punctuationMode: false;
    // Style
    color: hifi.colors.baseGray;
    Hifi.QmlCommerce {
        id: commerce;

        onAccountResult: {
            if (result.status === "success") {
                commerce.getKeyFilePathIfExists();
            } else {
                // unsure how to handle a failure here. We definitely cannot proceed.
            }
        }

        onLoginStatusResult: {
            if (!isLoggedIn && root.activeView !== "needsLogIn") {
                root.activeView = "needsLogIn";
            } else if (isLoggedIn) {
                root.activeView = "initialize";
                commerce.account();
            }
        }

        onKeyFilePathIfExistsResult: {
            if (path === "" && root.activeView !== "notSetUp") {
                root.activeView = "notSetUp";
            } else if (path !== "" && root.activeView === "initialize") {
                commerce.getSecurityImage();
            }
        }

        onSecurityImageResult: {
            securityImageResultReceived = true;
            if (!exists && root.activeView !== "notSetUp") { // "If security image is not set up"
                root.activeView = "notSetUp";
            } else if (exists && root.activeView === "initialize") {
                commerce.getWalletAuthenticatedStatus();
            } else if (exists) {
                // just set the source again (to be sure the change was noticed)
                securityImage.source = "";
                securityImage.source = "image://security/securityImage";
            }
        }

        onWalletAuthenticatedStatusResult: {
            if (!isAuthenticated && !passphraseModal.visible) {
                passphraseModal.visible = true;
            } else if (isAuthenticated) {
                root.activeView = "purchasesMain";
                commerce.inventory();
            }
        }

        onInventoryResult: {
            purchasesReceived = true;
            if (result.status !== 'success') {
                console.log("Failed to get purchases", result.message);
            } else {
                purchasesModel.clear();
                purchasesModel.append(result.data.assets);
                filteredPurchasesModel.clear();
                filteredPurchasesModel.append(result.data.assets);
            }
        }
    }

    HifiWallet.SecurityImageModel {
        id: securityImageModel;
    }

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        visible: !needsLogIn.visible;
        // Size
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "PURCHASES";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.bottom: parent.bottom;
            width: paintedWidth;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Security Image (TEMPORARY!)
        Image {
            id: securityImage;
            // Anchors
            anchors.top: parent.top;
            anchors.right: parent.right;
            anchors.verticalCenter: parent.verticalCenter;
            height: parent.height - 10;
            width: height;
            fillMode: Image.PreserveAspectFit;
            mipmap: true;
            cache: false;
            source: "image://security/securityImage";
        }
        Image {
            id: securityImageOverlay;
            source: "../wallet/images/lockOverlay.png";
            width: securityImage.width * 0.45;
            height: securityImage.height * 0.45;
            anchors.bottom: securityImage.bottom;
            anchors.right: securityImage.right;
            mipmap: true;
            opacity: 0.9;
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
        }
    }
    //
    // TITLE BAR END
    //

    Rectangle {
        id: initialize;
        visible: root.activeView === "initialize";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        color: hifi.colors.baseGray;

        Component.onCompleted: {
            securityImageResultReceived = false;
            purchasesReceived = false;
            commerce.getLoginStatus();
        }
    }

    HifiWallet.NeedsLogIn {
        id: needsLogIn;
        visible: root.activeView === "needsLogIn";
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        Connections {
            onSendSignalToWallet: {
                sendToScript(msg);
            }
        }
    }
    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            commerce.getLoginStatus();
        }
    }

    HifiWallet.PassphraseModal {
        id: passphraseModal;
        visible: false;
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        Connections {
            onSendSignalToParent: {
                sendToScript(msg);
            }
        }
    }

    //
    // "WALLET NOT SET UP" START
    //
    Item {
        id: notSetUp;
        visible: root.activeView === "notSetUp";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        RalewayRegular {
            id: notSetUpText;
            text: "<b>Your wallet isn't set up.</b><br><br>Set up your Wallet (no credit card necessary) to claim your <b>free HFC</b> " +
            "and get items from the Marketplace.";
            // Text size
            size: 24;
            // Anchors
            anchors.top: parent.top;
            anchors.bottom: notSetUpActionButtonsContainer.top;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }

        Item {
            id: notSetUpActionButtonsContainer;
            // Size
            width: root.width;
            height: 70;
            // Anchors
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 24;

            // "Cancel" button
            HifiControlsUit.Button {
                id: cancelButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: parent.width/2 - anchors.leftMargin*2;
                text: "Cancel"
                onClicked: {
                    sendToScript({method: 'purchases_backClicked', referrerURL: referrerURL});
                }
            }

            // "Set Up" button
            HifiControlsUit.Button {
                id: setUpButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: parent.width/2 - anchors.rightMargin*2;
                text: "Set Up Wallet"
                onClicked: {
                    sendToScript({method: 'checkout_setUpClicked'});
                }
            }
        }
    }
    //
    // "WALLET NOT SET UP" END
    //

    //
    // PURCHASES CONTENTS START
    //
    Item {
        id: purchasesContentsContainer;
        visible: root.activeView === "purchasesMain";
        // Anchors
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.right: parent.right;
        anchors.rightMargin: 4;
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 8;
        anchors.bottom: actionButtonsContainer.top;
        anchors.bottomMargin: 8;

        //
        // FILTER BAR START
        //
        Item {
            id: filterBarContainer;
            // Size
            height: 40;
            // Anchors
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.right: parent.right;
            anchors.rightMargin: 8;
            anchors.top: parent.top;
            anchors.topMargin: 4;

            HifiControlsUit.TextField {
                id: filterBar;
                property int previousLength: 0;
                anchors.fill: parent;
                placeholderText: "Filter";

                onTextChanged: {
                    if (filterBar.text.length < previousLength) {
                        filteredPurchasesModel.clear();

                        for (var i = 0; i < purchasesModel.count; i++) {
                            filteredPurchasesModel.append(purchasesModel.get(i));
                        }
                    }

                    for (var i = 0; i < filteredPurchasesModel.count; i++) {
                        if (filteredPurchasesModel.get(i).title.toLowerCase().indexOf(filterBar.text.toLowerCase()) === -1) {
                            filteredPurchasesModel.remove(i);
                            i--;
                        }
                    }
                    previousLength = filterBar.text.length;
                }

                onAccepted: {
                    focus = false;
                }
            }
        }
        //
        // FILTER BAR END
        //

        ListModel {
            id: purchasesModel;
        }
        ListModel {
            id: filteredPurchasesModel;
        }

        ListView {
            id: purchasesContentsList;
            visible: purchasesModel.count !== 0;
            clip: true;
            model: filteredPurchasesModel;
            // Anchors
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;
            delegate: PurchasedItem {
                itemName: title;
                itemId: id;
                itemPreviewImageUrl: preview;
                itemHref: root_file_url;
                anchors.topMargin: 12;
                anchors.bottomMargin: 12;

                Connections {
                    onSendToPurchases: {
                        if (msg.method === 'purchases_itemInfoClicked') {
                            sendToScript({method: 'purchases_itemInfoClicked', itemId: itemId});
                        }
                    }
                }
            }
        }

        Item {
            id: noPurchasesAlertContainer;
            visible: !purchasesContentsList.visible && root.purchasesReceived;
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;

            // Explanitory text
            RalewayRegular {
                id: haventPurchasedYet;
                text: "<b>You haven't purchased anything yet!</b><br><br>Get an item from <b>Marketplace</b> to add it to your <b>Purchases</b>.";
                // Text size
                size: 22;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 150;
                anchors.left: parent.left;
                anchors.leftMargin: 24;
                anchors.right: parent.right;
                anchors.rightMargin: 24;
                height: paintedHeight;
                // Style
                color: hifi.colors.faintGray;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
            }

            // "Set Up" button
            HifiControlsUit.Button {
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: haventPurchasedYet.bottom;
                anchors.topMargin: 20;
                anchors.horizontalCenter: parent.horizontalCenter;
                width: parent.width * 2 / 3;
                height: 50;
                text: "Visit Marketplace";
                onClicked: {
                    sendToScript({method: 'purchases_goToMarketplaceClicked'});
                }
            }
        }
    }
    //
    // PURCHASES CONTENTS END
    //

    //
    // ACTION BUTTONS START
    //
    Item {
        id: actionButtonsContainer;
        visible: purchasesContentsContainer.visible;
        // Size
        width: parent.width;
        height: 40;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: keyboard.top;
        anchors.bottomMargin: 8;

        // "Back" button
        HifiControlsUit.Button {
            id: backButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: parent.width/2 - anchors.leftMargin*2;
            text: "Back"
            onClicked: {
                sendToScript({method: 'purchases_backClicked', referrerURL: referrerURL});
            }
        }
    }
    //
    // ACTION BUTTONS END
    //

    HifiControlsUit.Keyboard {
        id: keyboard;
        raised: HMD.mounted && filterBar.focus;
        numeric: parent.punctuationMode;
        anchors {
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    //
    // Function Name: fromScript()
    //
    // Relevant Variables:
    // None
    //
    // Arguments:
    // message: The message sent from the JavaScript, in this case the Marketplaces JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from a script.
    //
    function fromScript(message) {
        switch (message.method) {
            case 'updatePurchases':
                referrerURL = message.referrerURL;
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
