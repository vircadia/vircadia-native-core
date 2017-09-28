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
import "../common" as HifiCommerceCommon

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "initialize";
    property string referrerURL: "";
    property bool securityImageResultReceived: false;
    property bool purchasesReceived: false;
    property bool punctuationMode: false;
    property bool canRezCertifiedItems: false;
    property bool pendingInventoryReply: true;
    // Style
    color: hifi.colors.white;
    Hifi.QmlCommerce {
        id: commerce;

        onLoginStatusResult: {
            if (!isLoggedIn && root.activeView !== "needsLogIn") {
                root.activeView = "needsLogIn";
            } else if (isLoggedIn) {
                root.activeView = "initialize";
                commerce.account();
            }
        }

        onAccountResult: {
            if (result.status === "success") {
                commerce.getKeyFilePathIfExists();
            } else {
                // unsure how to handle a failure here. We definitely cannot proceed.
            }
        }

        onKeyFilePathIfExistsResult: {
            if (path === "" && root.activeView !== "notSetUp") {
                root.activeView = "notSetUp";
                notSetUpTimer.start();
            } else if (path !== "" && root.activeView === "initialize") {
                commerce.getSecurityImage();
            }
        }

        onSecurityImageResult: {
            securityImageResultReceived = true;
            if (!exists && root.activeView !== "notSetUp") { // "If security image is not set up"
                root.activeView = "notSetUp";
                notSetUpTimer.start();
            } else if (exists && root.activeView === "initialize") {
                commerce.getWalletAuthenticatedStatus();
            }
        }

        onWalletAuthenticatedStatusResult: {
            if (!isAuthenticated && root.activeView !== "passphraseModal") {
                root.activeView = "passphraseModal";
            } else if (isAuthenticated) {
                sendToScript({method: 'purchases_getIsFirstUse'});
            }
        }

        onInventoryResult: {
            purchasesReceived = true;

            if (result.status !== 'success') {
                console.log("Failed to get purchases", result.message);
            } else {
                purchasesModel.clear();
                purchasesModel.append(result.data.assets);

                if (previousPurchasesModel.count !== 0) {
                    checkIfAnyItemStatusChanged();
                } else {
                    // Fill statusChanged default value
                    // Not doing this results in the default being true...
                    for (var i = 0; i < purchasesModel.count; i++) {
                        purchasesModel.setProperty(i, "statusChanged", false);
                    }
                }
                previousPurchasesModel.append(result.data.assets);

                buildFilteredPurchasesModel();

                if (root.pendingInventoryReply) {
                    inventoryTimer.start();
                }
            }

            root.pendingInventoryReply = false;
        }
    }

    Timer {
        id: notSetUpTimer;
        interval: 200;
        onTriggered: {
            sendToScript({method: 'checkout_walletNotSetUp'});
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;

        Connections {
            onSendToParent: {
                sendToScript(msg);
            }
        }
    }

    //
    // TITLE BAR START
    //
    HifiCommerceCommon.EmulatedMarketplaceHeader {
        id: titleBarContainer;
        z: 998;
        visible: !needsLogIn.visible;
        // Size
        width: parent.width;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        Connections {
            onSendToParent: {
                if (msg.method === 'needsLogIn' && root.activeView !== "needsLogIn") {
                    root.activeView = "needsLogIn";
                } else if (msg.method === 'showSecurityPicLightbox') {
                    lightboxPopup.titleText = "Your Security Pic";
                    lightboxPopup.bodyImageSource = msg.securityImageSource;
                    lightboxPopup.bodyText = lightboxPopup.securityPicBodyText;
                    lightboxPopup.button1text = "CLOSE";
                    lightboxPopup.button1method = "root.visible = false;"
                    lightboxPopup.button2text = "GO TO WALLET";
                    lightboxPopup.button2method = "sendToParent({method: 'purchases_openWallet'});";
                    lightboxPopup.visible = true;
                } else {
                    sendToScript(msg);
                }
            }
        }
    }
    //
    // TITLE BAR END
    //

    Rectangle {
        id: initialize;
        visible: root.activeView === "initialize";
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: -titleBarContainer.additionalDropdownHeight;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        color: hifi.colors.white;

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
        visible: root.activeView === "passphraseModal";
        anchors.fill: parent;
        titleBarText: "Purchases";
        titleBarIcon: hifi.glyphs.wallet;

        Connections {
            onSendSignalToParent: {
                if (msg.method === "authSuccess") {
                    root.activeView = "initialize";
                    sendToScript({method: 'purchases_getIsFirstUse'});
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    FirstUseTutorial {
        id: firstUseTutorial;
        visible: root.activeView === "firstUseTutorial";
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: -titleBarContainer.additionalDropdownHeight;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        Connections {
            onSendSignalToParent: {
                switch (message.method) {
                    case 'tutorial_skipClicked':
                    case 'tutorial_finished':
                        sendToScript({method: 'purchases_setIsFirstUse'});
                        root.activeView = "purchasesMain";
                        commerce.inventory();
                    break;
                }
            }
        }
    }

    //
    // PURCHASES CONTENTS START
    //
    Item {
        id: purchasesContentsContainer;
        visible: root.activeView === "purchasesMain";
        // Anchors
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 8 - titleBarContainer.additionalDropdownHeight;
        anchors.bottom: parent.bottom;

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
            anchors.rightMargin: 12;
            anchors.top: parent.top;
            anchors.topMargin: 4;

            RalewayRegular {
                id: myPurchasesText;
                anchors.top: parent.top;
                anchors.topMargin: 10;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 10;
                anchors.left: parent.left;
                anchors.leftMargin: 4;
                width: paintedWidth;
                text: "My Purchases";
                color: hifi.colors.baseGray;
                size: 28;
            }

            HifiControlsUit.TextField {
                id: filterBar;
                colorScheme: hifi.colorSchemes.faintGray;
                hasClearButton: true;
                hasRoundedBorder: true;
                anchors.left: myPurchasesText.right;
                anchors.leftMargin: 16;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                placeholderText: "filter items";

                onTextChanged: {
                    buildFilteredPurchasesModel();
                }

                onAccepted: {
                    focus = false;
                }
            }
        }
        //
        // FILTER BAR END
        //

        HifiControlsUit.Separator {
            id: separator;
            colorScheme: 1;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 16;
        }

        ListModel {
            id: purchasesModel;
        }
        ListModel {
            id: previousPurchasesModel;
        }
        ListModel {
            id: filteredPurchasesModel;
        }

        Rectangle {
            id: cantRezCertified;
            visible: !root.canRezCertifiedItems;
            color: "#FFC3CD";
            radius: 4;
            border.color: hifi.colors.redAccent;
            border.width: 1;
            anchors.top: separator.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: 80;

            HiFiGlyphs {
                id: lightningIcon;
                text: hifi.glyphs.lightning;
                // Size
                size: 36;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 18;
                anchors.left: parent.left;
                anchors.leftMargin: 12;
                horizontalAlignment: Text.AlignHCenter;
                // Style
                color: hifi.colors.lightGray;
            }

            RalewayRegular {
                text: "You don't have permission to rez certified items in this domain. " +
                '<b><font color="' + hifi.colors.blueAccent + '"><a href="#">Learn More</a></font></b>';
                // Text size
                size: 18;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 4;
                anchors.left: lightningIcon.right;
                anchors.leftMargin: 8;
                anchors.right: parent.right;
                anchors.rightMargin: 8;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 4;
                // Style
                color: hifi.colors.baseGray;
                wrapMode: Text.WordWrap;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
                
                onLinkActivated: {
                    lightboxPopup.titleText = "Rez Permission Required";
                    lightboxPopup.bodyText = "You don't have permission to rez certified items in this domain.<br><br>" +
                        "Use the <b>GOTO app</b> to visit another domain or <b>go to your own sandbox.</b>";
                    lightboxPopup.button1text = "CLOSE";
                    lightboxPopup.button1method = "root.visible = false;"
                    lightboxPopup.button2text = "OPEN GOTO";
                    lightboxPopup.button2method = "sendToParent({method: 'purchases_openGoTo'});";
                    lightboxPopup.visible = true;
                }
            }
        }

        ListView {
            id: purchasesContentsList;
            visible: purchasesModel.count !== 0;
            clip: true;
            model: filteredPurchasesModel;
            // Anchors
            anchors.top: root.canRezCertifiedItems ? separator.bottom : cantRezCertified.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;
            delegate: PurchasedItem {
                canRezCertifiedItems: root.canRezCertifiedItems;
                itemName: title;
                itemId: id;
                itemPreviewImageUrl: preview;
                itemHref: root_file_url;
                purchaseStatus: status;
                purchaseStatusChanged: statusChanged;
                anchors.topMargin: 12;
                anchors.bottomMargin: 12;

                Connections {
                    onSendToPurchases: {
                        if (msg.method === 'purchases_itemInfoClicked') {
                            sendToScript({method: 'purchases_itemInfoClicked', itemId: itemId});
                        } else if (msg.method === 'purchases_itemCertificateClicked') {
                            sendToScript(msg);
                        } else if (msg.method === "showInvalidatedLightbox") {
                            lightboxPopup.titleText = "Item Invalidated";
                            lightboxPopup.bodyText = 'Your item is marked "invalidated" because this item has been suspended ' +
                            "from the Marketplace due to a claim against its author.";
                            lightboxPopup.button1text = "CLOSE";
                            lightboxPopup.button1method = "root.visible = false;"
                            lightboxPopup.visible = true;
                        } else if (msg.method === "showPendingLightbox") {
                            lightboxPopup.titleText = "Item Pending";
                            lightboxPopup.bodyText = 'Your item is marked "pending" while your purchase is being confirmed. ' +
                            "Usually, purchases take about 90 seconds to confirm.";
                            lightboxPopup.button1text = "CLOSE";
                            lightboxPopup.button1method = "root.visible = false;"
                            lightboxPopup.visible = true;
                        } else if (msg.method === "setFilterText") {
                            filterBar.text = msg.filterText;
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
                text: "<b>You haven't purchased anything yet!</b><br><br>Get an item from <b>Marketplace</b> to add it to My Purchases.";
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
                color: hifi.colors.baseGray;
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

    onVisibleChanged: {
        if (!visible) {
            inventoryTimer.stop();
        }
    }

    Timer {
        id: inventoryTimer;
        interval: 90000;
        onTriggered: {
            if (root.activeView === "purchasesMain" && !root.pendingInventoryReply) {
                root.pendingInventoryReply = true;
                commerce.inventory();
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //

    function buildFilteredPurchasesModel() {
        filteredPurchasesModel.clear();
        for (var i = 0; i < purchasesModel.count; i++) {
            if (purchasesModel.get(i).title.toLowerCase().indexOf(filterBar.text.toLowerCase()) !== -1) {
                if (purchasesModel.get(i).status !== "confirmed") {
                    filteredPurchasesModel.insert(0, purchasesModel.get(i));
                } else {
                    filteredPurchasesModel.append(purchasesModel.get(i));
                }
            }
        }
    }

    function checkIfAnyItemStatusChanged() {
        var currentPurchasesModelId, currentPurchasesModelEdition, currentPurchasesModelStatus;
        var previousPurchasesModelStatus;
        for (var i = 0; i < purchasesModel.count; i++) {
            currentPurchasesModelId = purchasesModel.get(i).id;
            currentPurchasesModelEdition = purchasesModel.get(i).edition_number;
            currentPurchasesModelStatus = purchasesModel.get(i).status;

            for (var j = 0; j < previousPurchasesModel.count; j++) {
                previousPurchasesModelStatus = previousPurchasesModel.get(j).status;
                if (currentPurchasesModelId === previousPurchasesModel.get(j).id &&
                    currentPurchasesModelEdition === previousPurchasesModel.get(j).edition_number &&
                    currentPurchasesModelStatus !== previousPurchasesModelStatus) {
                    
                    purchasesModel.setProperty(i, "statusChanged", true);
                }
            }
        }
    }

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
                titleBarContainer.referrerURL = message.referrerURL;
                root.canRezCertifiedItems = message.canRezCertifiedItems;
                filterBar.text = message.filterText ? message.filterText : "";
            break;
            case 'purchases_getIsFirstUseResult':
                if (message.isFirstUseOfPurchases && root.activeView !== "firstUseTutorial") {
                    root.activeView = "firstUseTutorial";
                } else if (!message.isFirstUseOfPurchases && root.activeView === "initialize") {
                    root.activeView = "purchasesMain";
                    commerce.inventory();
                }
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
