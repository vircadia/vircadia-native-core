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
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;

    property string activeView: "walletHome";

    // Style
    color: hifi.colors.baseGray;
    Hifi.QmlCommerce {
        id: commerce;

        onSecurityImageResult: {
            if (!exists) { // "If security image is not set up"
                if (root.activeView !== "notSetUp") {
                    root.activeView = "notSetUp";
                }
            }
        }

        onKeyFilePathResult: {
            if (path === "" && root.activeView !== "notSetUp") {
                root.activeView = "notSetUp";
            }
        }
    }

    SecurityImageModel {
        id: securityImageModel;
    }

    Connections {
        target: walletSetupLightbox;
        onSendSignalToWallet: {
            if (msg.method === 'walletSetup_cancelClicked') {
                walletSetupLightbox.visible = false;
            } else if (msg.method === 'walletSetup_finished') {
                root.activeView = "walletHome";
            } else {
                sendToScript(msg);
            }
        }
    }
    Connections {
        target: notSetUp;
        onSendSignalToWallet: {
            if (msg.method === 'setUpClicked') {
                walletSetupLightbox.visible = true;
            }
        }
    }

    Rectangle {
        id: walletSetupLightboxContainer;
        visible: walletSetupLightbox.visible || passphraseSelectionLightbox.visible || securityImageSelectionLightbox.visible;
        z: 998;
        anchors.fill: parent;
        color: "black";
        opacity: 0.5;
    }
    WalletSetupLightbox {
        id: walletSetupLightbox;
        visible: false;
        z: 999;
        anchors.centerIn: walletSetupLightboxContainer;
        width: walletSetupLightboxContainer.width - 50;
        height: walletSetupLightboxContainer.height - 50;
    }
    PassphraseSelectionLightbox {
        id: passphraseSelectionLightbox;
        visible: false;
        z: 999;
        anchors.centerIn: walletSetupLightboxContainer;
        width: walletSetupLightboxContainer.width - 50;
        height: walletSetupLightboxContainer.height - 50;
    }
    SecurityImageSelectionLightbox {
        id: securityImageSelectionLightbox;
        visible: false;
        z: 999;
        anchors.centerIn: walletSetupLightboxContainer;
        width: walletSetupLightboxContainer.width - 50;
        height: walletSetupLightboxContainer.height - 50;
    }


    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        width: parent.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "WALLET";
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

    //
    // TAB CONTENTS START
    //
    NotSetUp {
        id: notSetUp;
        visible: root.activeView === "notSetUp";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: tabButtonsContainer.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
    }

    WalletHome {
        id: walletHome;
        visible: root.activeView === "walletHome";
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 16;
        anchors.bottom: tabButtonsContainer.top;
        anchors.bottomMargin: 16;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
    }

    SendMoney {
        id: sendMoney;
        visible: root.activeView === "sendMoney";
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 16;
        anchors.bottom: tabButtonsContainer.top;
        anchors.bottomMargin: 16;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
    }

    Security {
        id: security;
        visible: root.activeView === "security";
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 16;
        anchors.bottom: tabButtonsContainer.top;
        anchors.bottomMargin: 16;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
    }
    Connections {
        target: security;
        onSendSignalToWallet: {
            if (msg.method === 'walletSecurity_changePassphrase') {
                passphraseSelectionLightbox.visible = true;
            } else if (msg.method === 'walletSecurity_changeSecurityImage') {
                securityImageSelectionLightbox.visible = true;
            }
        }
    }

    Help {
        id: help;
        visible: root.activeView === "help";
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 16;
        anchors.bottom: tabButtonsContainer.top;
        anchors.bottomMargin: 16;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
    }


    //
    // TAB CONTENTS END
    //

    //
    // TAB BUTTONS START
    //
    Item {
        id: tabButtonsContainer;
        property int numTabs: 5;
        // Size
        width: root.width;
        height: 80;
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
            visible: !notSetUp.visible;
            color: root.activeView === "walletHome" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;

            RalewaySemiBold {
                text: "WALLET HOME";
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.fill: parent;
                anchors.leftMargin: 4;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.faintGray;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
            MouseArea {
                enabled: !walletSetupLightboxContainer.visible;
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    root.activeView = "walletHome";
                    tabButtonsContainer.resetTabButtonColors();
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = root.activeView === "walletHome" ? hifi.colors.blueAccent : hifi.colors.black;
            }

            onVisibleChanged: {
                if (visible) {
                    commerce.getSecurityImage();
                    commerce.balance();
                }
            }
        }

        // "SEND MONEY" tab button
        Rectangle {
            id: sendMoneyButtonContainer;
            visible: !notSetUp.visible;
            color: hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: walletHomeButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;

            RalewaySemiBold {
                text: "SEND MONEY";
                // Text size
                size: 14;
                // Anchors
                anchors.fill: parent;
                anchors.leftMargin: 4;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.lightGray50;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
        }

        // "EXCHANGE MONEY" tab button
        Rectangle {
            id: exchangeMoneyButtonContainer;
            visible: !notSetUp.visible;
            color: hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: sendMoneyButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;

            RalewaySemiBold {
                text: "EXCHANGE MONEY";
                // Text size
                size: 14;
                // Anchors
                anchors.fill: parent;
                anchors.leftMargin: 4;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.lightGray50;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
        }

        // "SECURITY" tab button
        Rectangle {
            id: securityButtonContainer;
            visible: !notSetUp.visible;
            color: root.activeView === "security" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: exchangeMoneyButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;

            RalewaySemiBold {
                text: "SECURITY";
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.fill: parent;
                anchors.leftMargin: 4;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.faintGray;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
            MouseArea {
                enabled: !walletSetupLightboxContainer.visible;
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    root.activeView = "security";
                    tabButtonsContainer.resetTabButtonColors();
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = root.activeView === "security" ? hifi.colors.blueAccent : hifi.colors.black;
            }

            onVisibleChanged: {
                if (visible) {
                    commerce.getSecurityImage();
                    commerce.getKeyFilePath();
                }
            }
        }

        // "HELP" tab button
        Rectangle {
            id: helpButtonContainer;
            visible: !notSetUp.visible;
            color: root.activeView === "help" ? hifi.colors.blueAccent : hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: securityButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;

            RalewaySemiBold {
                text: "HELP";
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.fill: parent;
                anchors.leftMargin: 4;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.faintGray;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
            MouseArea {
                enabled: !walletSetupLightboxContainer.visible;
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
            default:
                console.log('Unrecognized message from wallet.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
