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
    // Style
    color: hifi.colors.baseGray;
    Hifi.QmlCommerce {
        id: commerce;
        onBalanceResult: {
            if (failureMessage.length) {
                console.log("Failed to get balance", failureMessage);
            } else {
                hfcBalanceText.text = balance;
            }
        }
    }
    Connections {
        target: walletSetupLightbox;
        onSendSignalToWallet: {
            sendToScript(msg);
        }
    }

    Rectangle {
        id: walletSetupLightboxContainer;
        visible: walletSetupLightbox.visible;
        z: 998;
        anchors.fill: parent;
        color: "black";
        opacity: 0.5;
    }
    WalletSetupLightbox {
        id: walletSetupLightbox;
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
            color: hifi.colors.lightGrayText;
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



    //
    // TAB CONTENTS END
    //
    
    //
    // TAB BUTTONS START
    //
    Item {
        id: tabButtonsContainer;
        property int numTabs: 4;
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

        // "ACCOUNT HOME" tab button
        Rectangle {
            id: accountHomeButtonContainer;
            color: hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;

            RalewaySemiBold {
                text: "ACCOUNT HOME";
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.fill: parent;
                anchors.leftMargin: 4;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.lightGrayText;
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
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = hifi.colors.black;
            }
        }

        // "SEND MONEY" tab button
        Rectangle {
            id: sendMoneyButtonContainer;
            color: hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: accountHomeButtonContainer.right;
            anchors.bottom: parent.bottom;
            width: parent.width / tabButtonsContainer.numTabs;

            RalewaySemiBold {
                text: "SEND MONEY";
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.fill: parent;
                anchors.leftMargin: 4;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.lightGrayText;
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
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = hifi.colors.black;
            }
        }

        // "SECURITY" tab button
        Rectangle {
            id: securityButtonContainer;
            color: hifi.colors.black;
            anchors.top: parent.top;
            anchors.left: sendMoneyButtonContainer.right;
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
                color: hifi.colors.lightGrayText;
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
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = hifi.colors.black;
            }
        }

        // "HELP" tab button
        Rectangle {
            id: helpButtonContainer;
            color: hifi.colors.black;
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
                color: hifi.colors.lightGrayText;
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
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = hifi.colors.black;
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
