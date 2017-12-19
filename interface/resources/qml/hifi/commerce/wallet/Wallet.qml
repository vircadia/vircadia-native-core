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
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls
import "../common" as HifiCommerceCommon

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;

    property string activeView: "initialize";
    property bool keyboardRaised: false;
    property bool isPassword: false;

    Image {
        anchors.fill: parent;
        source: "images/wallet-bg.jpg";
    }

    Hifi.QmlCommerce {
        id: commerce;

        onWalletStatusResult: {
            if (walletStatus === 0) {
                if (root.activeView !== "needsLogIn") {
                    root.activeView = "needsLogIn";
                }
            } else if (walletStatus === 1) {
                if (root.activeView !== "walletSetup") {
                    root.activeView = "walletSetup";
                    commerce.resetLocalWalletOnly();
                    var timestamp = new Date();
                    walletSetup.startingTimestamp = timestamp;
                    walletSetup.setupAttemptID = generateUUID();
                    UserActivityLogger.commerceWalletSetupStarted(timestamp, setupAttemptID, walletSetup.setupFlowVersion, walletSetup.referrer ? walletSetup.referrer : "wallet app",
                        (AddressManager.placename || AddressManager.hostname || '') + (AddressManager.pathname ? AddressManager.pathname.match(/\/[^\/]+/)[0] : ''));
                }
            } else if (walletStatus === 2) {
                if (root.activeView !== "passphraseModal") {
                    root.activeView = "passphraseModal";
                    UserActivityLogger.commercePassphraseEntry("wallet app");
                }
            } else if (walletStatus === 3) {
                root.activeView = "walletHome";
                commerce.getSecurityImage();
            } else {
                console.log("ERROR in Wallet.qml: Unknown wallet status: " + walletStatus);
            }
        }

        onLoginStatusResult: {
            if (!isLoggedIn && root.activeView !== "needsLogIn") {
                root.activeView = "needsLogIn";
            } else if (isLoggedIn) {
                commerce.getWalletStatus();
            }
        }

        onSecurityImageResult: {
            if (exists) {
                titleBarSecurityImage.source = "";
                titleBarSecurityImage.source = "image://security/securityImage";
            }
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
        HiFiGlyphs {
            id: walletIcon;
            text: hifi.glyphs.wallet;
            // Size
            size: parent.height * 0.8;
            // Anchors
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.verticalCenter: parent.verticalCenter;
            // Style
            color: hifi.colors.blueHighlight;
        }

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "WALLET";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.top: parent.top;
            anchors.left: walletIcon.right;
            anchors.leftMargin: 4;
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
            visible: titleBarSecurityImage.source !== "" && !securityImageChange.visible;
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
                    lightboxPopup.button1method = "root.visible = false;"
                    lightboxPopup.visible = true;
                }
            }
        }
    }
    //
    // TITLE BAR END
    //

    WalletSetup {
        id: walletSetup;
        visible: root.activeView === "walletSetup";
        z: 997;
        anchors.fill: parent;

        Connections {
            onSendSignalToWallet: {
                if (msg.method === 'walletSetup_finished') {
                    if (msg.referrer === '' || msg.referrer === 'marketplace cta') {
                        root.activeView = "initialize";
                        commerce.getWalletStatus();
                    } else if (msg.referrer === 'purchases') {
                        sendToScript({method: 'goToPurchases'});
                    } else {
                        sendToScript({method: 'goToMarketplaceItemPage', itemId: msg.referrer});
                    }
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
                if (msg.method === 'walletSetup_raiseKeyboard') {
                    root.keyboardRaised = true;
                    root.isPassword = msg.isPasswordField;
                } else if (msg.method === 'walletSetup_lowerKeyboard') {
                    root.keyboardRaised = false;
                } else if (msg.method === 'walletSecurity_changePassphraseCancelled') {
                    root.activeView = "security";
                } else if (msg.method === 'walletSecurity_changePassphraseSuccess') {
                    root.activeView = "security";
                } else {
                    sendToScript(msg);
                }
            }
        }
    }
    SecurityImageChange {
        id: securityImageChange;
        visible: root.activeView === "securityImageChange";
        z: 997;
        anchors.top: titleBarContainer.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        Connections {
            onSendSignalToWallet: {
                if (msg.method === 'walletSecurity_changeSecurityImageCancelled') {
                    root.activeView = "security";
                } else if (msg.method === 'walletSecurity_changeSecurityImageSuccess') {
                    root.activeView = "security";
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
            commerce.getWalletStatus();
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
            commerce.getLoginStatus();
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
                    commerce.getWalletStatus();
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
                sendToScript(msg);
            }
        }
    }

    SendMoney {
        id: sendMoney;
        visible: root.activeView === "sendMoney";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: tabButtonsContainer.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
    }

    Security {
        id: security;
        visible: root.activeView === "security";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: tabButtonsContainer.top;
        anchors.left: parent.left;
        anchors.right: parent.right;

        Connections {
            onSendSignalToWallet: {
                if (msg.method === 'walletSecurity_changePassphrase') {
                    root.activeView = "passphraseChange";
                    passphraseChange.clearPassphraseFields();
                    passphraseChange.resetSubmitButton();
                } else if (msg.method === 'walletSecurity_changeSecurityImage') {
                    securityImageChange.initModel();
                    root.activeView = "securityImageChange";
                }
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

        Connections {
            onSendSignalToWallet: {
                if (msg.method === 'walletReset' || msg.method === 'passphraseReset') {
                    sendToScript(msg);
                }
            }
        }
    }


    //
    // TAB CONTENTS END
    //

    //
    // TAB BUTTONS START
    //
    Item {
        id: tabButtonsContainer;
        visible: !needsLogIn.visible && root.activeView !== "passphraseChange" && root.activeView !== "securityImageChange";
        property int numTabs: 5;
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
            visible: !walletSetup.visible;
            color: root.activeView === "walletHome" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;
        
            HiFiGlyphs {
                id: homeTabIcon;
                text: hifi.glyphs.home2;
                // Size
                size: 50;
                // Anchors
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top;
                anchors.topMargin: -2;
                // Style
                color: root.activeView === "walletHome" || walletHomeTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
            }

            RalewaySemiBold {
                text: "WALLET HOME";
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
                color: root.activeView === "walletHome" || walletHomeTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }
            MouseArea {
                id: walletHomeTabMouseArea;
                anchors.fill: parent;
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
            visible: !walletSetup.visible;
            color: hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: walletHomeButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;
        
            HiFiGlyphs {
                id: exchangeMoneyTabIcon;
                text: hifi.glyphs.leftRightArrows;
                // Size
                size: 50;
                // Anchors
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top;
                anchors.topMargin: -2;
                // Style
                color: hifi.colors.lightGray50;
            }

            RalewaySemiBold {
                text: "EXCHANGE MONEY";
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
                color: hifi.colors.lightGray50;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }
        }


        // "SEND MONEY" tab button
        Rectangle {
            id: sendMoneyButtonContainer;
            visible: !walletSetup.visible;
            color: hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: exchangeMoneyButtonContainer.right;
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
                color: hifi.colors.lightGray50;
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
                color: hifi.colors.lightGray50;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }
        }

        // "SECURITY" tab button
        Rectangle {
            id: securityButtonContainer;
            visible: !walletSetup.visible;
            color: root.activeView === "security" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: sendMoneyButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;
        
            HiFiGlyphs {
                id: securityTabIcon;
                text: hifi.glyphs.lock;
                // Size
                size: 38;
                // Anchors
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top;
                anchors.topMargin: 2;
                // Style
                color: root.activeView === "security" || securityTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
            }

            RalewaySemiBold {
                text: "SECURITY";
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
                color: root.activeView === "security" || securityTabMouseArea.containsMouse ? hifi.colors.white : hifi.colors.blueHighlight;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }
            MouseArea {
                id: securityTabMouseArea;
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    root.activeView = "security";
                    tabButtonsContainer.resetTabButtonColors();
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = root.activeView === "security" ? hifi.colors.blueAccent : hifi.colors.black;
            }
        }
        
        // "HELP" tab button
        Rectangle {
            id: helpButtonContainer;
            visible: !walletSetup.visible;
            color: root.activeView === "help" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: securityButtonContainer.right;
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
            securityButtonContainer.color = hifi.colors.black;
            helpButtonContainer.color = hifi.colors.black;
            if (root.activeView === "walletHome") {
                walletHomeButtonContainer.color = hifi.colors.blueAccent;
            } else if (root.activeView === "sendMoney") {
                sendMoneyButtonContainer.color = hifi.colors.blueAccent;
            } else if (root.activeView === "security") {
                securityButtonContainer.color = hifi.colors.blueAccent;
            } else if (root.activeView === "help") {
                helpButtonContainer.color = hifi.colors.blueAccent;
            }
        }
    }
    //
    // TAB BUTTONS END
    //

    Item {
        id: keyboardContainer;
        z: 998;
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
            break;
            case 'inspectionCertificate_resetCert':
                // NOP
            break;
            default:
                console.log('Unrecognized message from wallet.js:', JSON.stringify(message));
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
    //
    // FUNCTION DEFINITIONS END
    //
}
