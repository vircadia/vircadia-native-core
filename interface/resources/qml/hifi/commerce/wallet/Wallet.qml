//
//  Wallet.qml
//  qml/hifi/commerce/wallet
//
//  Wallet
//
//  Created by Zach Fox on 2017-08-17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtGraphicalEffects 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls
import "../common" as HifiCommerceCommon
import "../common/sendAsset"
import "../.." as HifiCommon
import "../purchases" as HifiPurchases
import "../inspectionCertificate" as HifiInspectionCertificate

Rectangle {
    HifiConstants { id: hifi; }

    id: root;

    property string activeView: "initialize";
    property string initialActiveViewAfterStatus5: "walletInventory";
    property bool keyboardRaised: false;
    property bool isPassword: false;

    anchors.fill: (typeof parent === undefined) ? undefined : parent;

    Image {
        anchors.fill: parent;
        source: "images/wallet-bg.jpg";
    }

    Component.onDestruction: {
        KeyboardScriptingInterface.raised = false;
    }

    Connections {
        target: Commerce;

        onWalletStatusResult: {
            if (walletStatus === 0) {
                if (root.activeView !== "needsLogIn") {
                    root.activeView = "needsLogIn";
                }
            } else if (walletStatus === 1) {
                if (root.activeView !== "walletSetup") {
                    walletResetSetup();
                }
            } else if (walletStatus === 2) {
                if (root.activeView != "preexisting") {
                    root.activeView = "preexisting";
                }
            } else if (walletStatus === 3) {
                if (root.activeView != "conflicting") {
                    root.activeView = "conflicting";
                }
            } else if (walletStatus === 4) {
                if (root.activeView !== "passphraseModal") {
                    root.activeView = "passphraseModal";
                    UserActivityLogger.commercePassphraseEntry("wallet app");
                }
            } else if (walletStatus === 5) {
                if (root.activeView !== "walletSetup") {
                    root.activeView = root.initialActiveViewAfterStatus5;
                    Commerce.getAvailableUpdates();
                    Commerce.getSecurityImage();
                }
            } else {
                console.log("ERROR in Wallet.qml: Unknown wallet status: " + walletStatus);
            }
        }

        onLoginStatusResult: {
            if (!isLoggedIn && root.activeView !== "needsLogIn") {
                root.activeView = "needsLogIn";
            } else if (isLoggedIn) {
                Commerce.getWalletStatus();
            }
        }

        onSecurityImageResult: {
            if (exists) {
                titleBarSecurityImage.source = "";
                titleBarSecurityImage.source = "image://security/securityImage";
            }
        }

        onAvailableUpdatesResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Available Updates", result.data.message);
            } else {
                exchangeMoneyButtonContainer.messagesWaiting = result.data.updates.length > 0;
            }
        }
    }

    onActiveViewChanged: {
        if (activeView === "walletHome") {
            walletHomeButtonContainer.messagesWaiting = false;
            sendToScript({method: 'clearShouldShowDotHistory'});
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;
    }

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        visible: !needsLogIn.visible;
        // Size
        width: parent.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // Wallet icon
        Image {
            id: walletIcon;
            source: "../../../../icons/tablet-icons/inventory-a.svg";
            height: parent.height * 0.5;
            width: walletIcon.height;
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.verticalCenter: parent.verticalCenter;
            visible: false; // When we use a white .svg instead of a glyph with color property, we set to invisible and use the following ColorOverlay.
        }
        ColorOverlay {
            anchors.fill: walletIcon;
            source: walletIcon;
            color: hifi.colors.blueHighlight;
        }

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "INVENTORY";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.top: parent.top;
            anchors.left: walletIcon.right;
            anchors.leftMargin: 6;
            anchors.bottom: parent.bottom;
            width: paintedWidth;
            // Style
            color: hifi.colors.white;
            // Alignment
            verticalAlignment: Text.AlignVCenter;
        }

        Image {
            id: titleBarSecurityImage;
            source: "";
            visible: titleBarSecurityImage.source !== "";
            anchors.right: parent.right;
            anchors.rightMargin: 6;
            anchors.top: parent.top;
            anchors.topMargin: 6;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 6;
            width: height;
            mipmap: true;
            cache: false;

            MouseArea {
                enabled: titleBarSecurityImage.visible;
                anchors.fill: parent;
                onClicked: {
                    lightboxPopup.titleText = "Your Security Pic";
                    lightboxPopup.bodyImageSource = titleBarSecurityImage.source;
                    lightboxPopup.bodyText = lightboxPopup.securityPicBodyText;
                    lightboxPopup.button1text = "CLOSE";
                    lightboxPopup.button1method = function() {
                        lightboxPopup.visible = false;
                    }
                    lightboxPopup.visible = true;
                }
            }
        }
    }
    //
    // TITLE BAR END
    //

    WalletChoice {
        id: walletChoice;
        proceedFunction: function (isReset) {
            console.log("WalletChoice", isReset ? "Reset wallet." : "Trying again with new wallet.");
            Commerce.setSoftReset();
            if (isReset) {
                walletResetSetup();
            } else {
                Commerce.clearWallet();
                var msg = { referrer: walletChoice.referrer }
                followReferrer(msg);
            }
        }
        copyFunction: Commerce.copyKeyFileFrom;
        z: 997;
        visible: (root.activeView === "preexisting") || (root.activeView === "conflicting");
        activeView: root.activeView;
        anchors.fill: parent;
    }

    WalletSetup {
        id: walletSetup;
        visible: root.activeView === "walletSetup";
        z: 997;
        anchors.fill: parent;

        Connections {
            onSendSignalToWallet: {
                if (msg.method === 'walletSetup_finished') {
                    followReferrer(msg);
                } else if (msg.method === 'walletSetup_raiseKeyboard') {
                    root.keyboardRaised = true;
                    root.isPassword = msg.isPasswordField;
                } else if (msg.method === 'walletSetup_lowerKeyboard') {
                    root.keyboardRaised = false;
                    root.isPassword = msg.isPasswordField;
                } else {
                    sendToScript(msg);
                }
            }
        }
    }
    PassphraseChange {
        id: passphraseChange;
        visible: root.activeView === "passphraseChange";
        z: 997;
        anchors.top: titleBarContainer.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        Connections {
            onSendSignalToWallet: {
                if (passphraseChange.visible) {
                    if (msg.method === 'walletSetup_raiseKeyboard') {
                        root.keyboardRaised = true;
                        root.isPassword = msg.isPasswordField;
                    } else if (msg.method === 'walletSetup_lowerKeyboard') {
                        root.keyboardRaised = false;
                    } else {
                        sendToScript(msg);
                    }
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    //
    // TAB CONTENTS START
    //

    Rectangle {
        id: initialize;
        visible: root.activeView === "initialize";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        color: hifi.colors.baseGray;

        Component.onCompleted: {
            Commerce.getWalletStatus();
        }
    }

    NeedsLogIn {
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
            Commerce.getLoginStatus();
        }
    }

    PassphraseModal {
        id: passphraseModal;
        visible: root.activeView === "passphraseModal";
        anchors.fill: parent;
        titleBarText: "Wallet";
        titleBarIcon: hifi.glyphs.wallet;

        Connections {
            onSendSignalToParent: {
                if (msg.method === "authSuccess") {
                    Commerce.getWalletStatus();
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    WalletHome {
        id: walletHome;
        visible: root.activeView === "walletHome";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: tabButtonsContainer.top;
        anchors.left: parent.left;
        anchors.right: parent.right;

        Connections {
            onSendSignalToWallet: {
                if (msg.method === 'transactionHistory_usernameLinkClicked') {
                    userInfoViewer.url = msg.usernameLink;
                    userInfoViewer.visible = true;
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    HifiInspectionCertificate.InspectionCertificate {
        id: inspectionCertificate;
        z: 998;
        visible: false;
        anchors.fill: parent;

        Connections {
            onSendToScript: {
                sendToScript(message);
            }
        }
    }

    HifiPurchases.Purchases {
        id: walletInventory;
        visible: root.activeView === "walletInventory";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: !WalletScriptingInterface.limitedCommerce ? tabButtonsContainer.top : parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        Connections {
            onSendToScript: {
                if (message.method === 'purchases_itemCertificateClicked') {
                    inspectionCertificate.visible = true;
                    inspectionCertificate.isLightbox = true;
                    sendToScript(message);
                } else {
                    sendToScript(message);
                }
            }
        }
    }

    HifiCommon.RootHttpRequest {
        id: http;
    }

    SendAsset {
        id: sendMoney;
        http: http;
        listModelName: "Send Money Connections";
        z: 997;
        visible: root.activeView === "sendMoney";
        keyboardContainer: root;
        anchors.fill: parent;
        parentAppTitleBarHeight: titleBarContainer.height;
        parentAppNavBarHeight: tabButtonsContainer.height;

        Connections {
            onSendSignalToParent: {
                sendToScript(msg);
            }
        }
    }

    Help {
        id: help;
        visible: root.activeView === "help";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: tabButtonsContainer.top;
        anchors.left: parent.left;
        anchors.right: parent.right;

    }


    //
    // TAB CONTENTS END
    //

    //
    // TAB BUTTONS START
    //
    Item {
        id: tabButtonsContainer;
        visible: !needsLogIn.visible && root.activeView !== "passphraseChange" && sendMoney.currentActiveView !== "sendAssetStep" && !WalletScriptingInterface.limitedCommerce;
        property int numTabs: 4;
        // Size
        width: root.width;
        height: 90;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: parent.top;
        }

        // "WALLET HOME" tab button
        Rectangle {
            id: walletHomeButtonContainer;
            property bool messagesWaiting: false;
            visible: !walletSetup.visible;
            color: root.activeView === "walletHome" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: exchangeMoneyButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;
        
            HiFiGlyphs {
                id: homeTabIcon;
                text: hifi.glyphs.leftRightArrows;
                // Size
                size: 50;
                // Anchors
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top;
                anchors.topMargin: -2;
                // Style
                color: WalletScriptingInterface.limitedCommerce ? hifi.colors.lightGray50 : ((root.activeView === "walletHome" || walletHomeTabMouseArea.containsMouse) ? hifi.colors.white : hifi.colors.blueHighlight);
            }

            Rectangle {
                id: recentActivityMessagesWaitingLight;
                visible: parent.messagesWaiting;
                anchors.right: homeTabIcon.left;
                anchors.rightMargin: -4;
                anchors.top: homeTabIcon.top;
                anchors.topMargin: 16;
                height: 10;
                width: height;
                radius: height/2;
                color: "red";
            }

            RalewaySemiBold {
                text: "RECENT ACTIVITY";
                // Text size
                size: 16;
                // Anchors
                anchors.bottom: parent.bottom;
                height: parent.height/2;
                anchors.left: parent.left;
                anchors.leftMargin: 4;
                anchors.right: parent.right;
                anchors.rightMargin: 4;
                // Style
                color: WalletScriptingInterface.limitedCommerce ? hifi.colors.lightGray50 : ((root.activeView === "walletHome" || walletHomeTabMouseArea.containsMouse) ? hifi.colors.white : hifi.colors.blueHighlight);
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }
            MouseArea {
                id: walletHomeTabMouseArea;
                anchors.fill: parent;
                enabled: !WalletScriptingInterface.limitedCommerce;
                hoverEnabled: enabled;
                onClicked: {
                    root.activeView = "walletHome";
                    tabButtonsContainer.resetTabButtonColors();
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = root.activeView === "walletHome" ? hifi.colors.blueAccent : hifi.colors.black;
            }
        }

        // "EXCHANGE MONEY" tab button
        Rectangle {
            id: exchangeMoneyButtonContainer;
            property bool messagesWaiting: false;

            visible: !walletSetup.visible;
            color: root.activeView === "walletInventory" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;
        
            Image {
                id: exchangeMoneyTabIcon;
                source: "images/items-tab-a.svg";
                height: 25;
                width: exchangeMoneyTabIcon.height;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top;
                anchors.topMargin: 10;
                visible: false; // When we use a white .svg instead of a glyph with color property, we set to invisible and use the following ColorOverlay.
            }
            ColorOverlay {
                anchors.fill: exchangeMoneyTabIcon;
                source: exchangeMoneyTabIcon;
                color: root.activeView === "walletInventory" || inventoryTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
            }

            Rectangle {
                id: exchangeMoneyMessagesWaitingLight;
                visible: parent.messagesWaiting;
                anchors.left: parent.left;
                anchors.leftMargin: 16;
                anchors.top: exchangeMoneyTabIcon.top;
                anchors.topMargin: 4;
                height: 10;
                width: height;
                radius: height/2;
                color: "red";
            }

            RalewaySemiBold {
                text: "ITEMS";
                // Text size
                size: 16;
                // Anchors
                anchors.bottom: parent.bottom;
                height: parent.height/2;
                anchors.left: parent.left;
                anchors.leftMargin: 4;
                anchors.right: parent.right;
                anchors.rightMargin: 4;
                // Style
                color: root.activeView === "walletInventory" || inventoryTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }

            MouseArea {
                id: inventoryTabMouseArea;
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    root.activeView = "walletInventory";
                    tabButtonsContainer.resetTabButtonColors();
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = root.activeView === "walletInventory" ? hifi.colors.blueAccent : hifi.colors.black;
            }
        }


        // "SEND MONEY" tab button
        Rectangle {
            id: sendMoneyButtonContainer;
            visible: !walletSetup.visible;
            color: root.activeView === "sendMoney" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: walletHomeButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;
        
            HiFiGlyphs {
                id: sendMoneyTabIcon;
                text: hifi.glyphs.paperPlane;
                // Size
                size: 46;
                // Anchors
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top;
                anchors.topMargin: -2;
                // Style
                color: WalletScriptingInterface.limitedCommerce ? hifi.colors.lightGray50 : ((root.activeView === "sendMoney" || sendMoneyTabMouseArea.containsMouse) ? hifi.colors.white : hifi.colors.blueHighlight);
            }

            RalewaySemiBold {
                text: "SEND MONEY";
                // Text size
                size: 16;
                // Anchors
                anchors.bottom: parent.bottom;
                height: parent.height/2;
                anchors.left: parent.left;
                anchors.leftMargin: 4;
                anchors.right: parent.right;
                anchors.rightMargin: 4;
                // Style
                color: WalletScriptingInterface.limitedCommerce ? hifi.colors.lightGray50 : ((root.activeView === "sendMoney" || sendMoneyTabMouseArea.containsMouse) ? hifi.colors.white : hifi.colors.blueHighlight);
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }

            MouseArea {
                id: sendMoneyTabMouseArea;
                anchors.fill: parent;
                enabled: !WalletScriptingInterface.limitedCommerce;
                hoverEnabled: enabled;
                onClicked: {
                    root.activeView = "sendMoney";
                    tabButtonsContainer.resetTabButtonColors();
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = root.activeView === "sendMoney" ? hifi.colors.blueAccent : hifi.colors.black;
            }
        }

        // "HELP" tab button
        Rectangle {
            id: helpButtonContainer;
            visible: !walletSetup.visible;
            color: root.activeView === "help" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: sendMoneyButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;
        
            HiFiGlyphs {
                id: helpTabIcon;
                text: hifi.glyphs.question;
                // Size
                size: 55;
                // Anchors
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top;
                anchors.topMargin: -6;
                // Style
                color: root.activeView === "help" || helpTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
            }

            RalewaySemiBold {
                text: "HELP";
                // Text size
                size: 16;
                // Anchors
                anchors.bottom: parent.bottom;
                height: parent.height/2;
                anchors.left: parent.left;
                anchors.leftMargin: 4;
                anchors.right: parent.right;
                anchors.rightMargin: 4;
                // Style
                color: root.activeView === "help" || helpTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }
            MouseArea {
                id: helpTabMouseArea;
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    root.activeView = "help";
                    tabButtonsContainer.resetTabButtonColors();
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = root.activeView === "help" ? hifi.colors.blueAccent : hifi.colors.black;
            }
        }


        function resetTabButtonColors() {
            walletHomeButtonContainer.color = hifi.colors.black;
            sendMoneyButtonContainer.color = hifi.colors.black;
            helpButtonContainer.color = hifi.colors.black;
            exchangeMoneyButtonContainer.color = hifi.colors.black;
            if (root.activeView === "walletHome") {
                walletHomeButtonContainer.color = hifi.colors.blueAccent;
            } else if (root.activeView === "sendMoney") {
                sendMoneyButtonContainer.color = hifi.colors.blueAccent;
            } else if (root.activeView === "help") {
                helpButtonContainer.color = hifi.colors.blueAccent;
            } else if (root.activeView == "walletInventory") {
                exchangeMoneyButtonContainer.color = hifi.colors.blueAccent;
            }
        }
    }
    //
    // TAB BUTTONS END
    //

    HifiControls.TabletWebView {
        id: userInfoViewer;
        z: 998;
        anchors.fill: parent;
        visible: false;
    }

    Item {
        id: keyboardContainer;
        z: 999;
        visible: keyboard.raised;
        property bool punctuationMode: false;
        anchors {
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }

        HifiControlsUit.Keyboard {
            id: keyboard;
            raised: HMD.mounted && root.keyboardRaised;
            numeric: parent.punctuationMode;
            password: root.isPassword;
            anchors {
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
            }
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
    // message: The message sent from the JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from a script.
    //
    function fromScript(message) {
        switch (message.method) {
            case 'updateWalletReferrer':
                walletSetup.referrer = message.referrer;
                walletChoice.referrer = message.referrer;
            break;
            case 'inspectionCertificate_resetCert':
                // NOP
            break;
            case 'updateConnections':
                sendMoney.updateConnections(message.connections);
                walletInventory.fromScript(message);
            break;
            case 'selectRecipient':
            case 'updateSelectedRecipientUsername':
                sendMoney.fromScript(message);
                walletInventory.fromScript(message);
            break;
            case 'http.response':
                http.handleHttpResponse(message);
                // Duplicate handler is required because we don't track referrer for `http`
                walletInventory.fromScript(message);
            break;
            case 'palIsStale':
            case 'avatarDisconnected':
                // Because we don't have "channels" for sending messages to a specific QML object, the messages are broadcast to all QML Items. If an Item of yours happens to be visible when some script sends a message with a method you don't expect, you'll get "Unrecognized message..." logs.
            break;
            case 'inspectionCertificate_setCertificateId':
                inspectionCertificate.fromScript(message);
            break;
            case 'updatePurchases':
            case 'purchases_showMyItems':
            case 'updateWearables':
                walletInventory.fromScript(message);
            break;
            case 'updateRecentActivityMessageLight':
                walletHomeButtonContainer.messagesWaiting = message.messagesWaiting;
            break;
            case 'checkout_openRecentActivity':
                if (root.activeView === "initialize") {
                    root.initialActiveViewAfterStatus5 = "walletHome";
                } else {
                    root.activeView = "walletHome";
                }
            break;
            default:
                console.log('Wallet.qml: Unrecognized message from wallet.js');
        }
    }
    signal sendToScript(var message);

    // generateUUID() taken from:
    // https://stackoverflow.com/a/8809472
    function generateUUID() { // Public Domain/MIT
        var d = new Date().getTime();
        if (typeof performance !== 'undefined' && typeof performance.now === 'function'){
            d += performance.now(); //use high-precision timer if available
        }
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
            var r = (d + Math.random() * 16) % 16 | 0;
            d = Math.floor(d / 16);
            return (c === 'x' ? r : (r & 0x3 | 0x8)).toString(16);
        });
    }

    function walletResetSetup() {
        /* Bypass all this and do it automatically
        root.activeView = "walletSetup";
        var timestamp = new Date();
        walletSetup.startingTimestamp = timestamp;
        walletSetup.setupAttemptID = generateUUID();
        UserActivityLogger.commerceWalletSetupStarted(timestamp, walletSetup.setupAttemptID, walletSetup.setupFlowVersion, walletSetup.referrer ? walletSetup.referrer : "wallet app",
            (AddressManager.placename || AddressManager.hostname || '') + (AddressManager.pathname ? AddressManager.pathname.match(/\/[^\/]+/)[0] : ''));
            */

        var randomNumber = Math.floor(Math.random() * 34) + 1;
        var securityImagePath = "images/" + addLeadingZero(randomNumber) + ".jpg";
        Commerce.getWalletAuthenticatedStatus(); // before writing security image, ensures that salt/account password is set.
        Commerce.chooseSecurityImage(securityImagePath);
        Commerce.generateKeyPair();
        followReferrer({ referrer: walletSetup.referrer });
    }

    function addLeadingZero(n) {
        return n < 10 ? '0' + n : '' + n;
    }

    function followReferrer(msg) {
        if (msg.referrer === '') {
            root.activeView = "initialize";
            Commerce.getWalletStatus();
        } else if (msg.referrer === 'purchases') {
            root.activeView = "walletInventory";
            tabButtonsContainer.resetTabButtonColors();
        } else if (msg.referrer === 'marketplace cta' || msg.referrer === 'mainPage') {
            sendToScript({method: 'goToMarketplaceMainPage', itemId: msg.referrer});
        } else {
            sendToScript({method: 'goToMarketplaceItemPage', itemId: msg.referrer});
        }
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
