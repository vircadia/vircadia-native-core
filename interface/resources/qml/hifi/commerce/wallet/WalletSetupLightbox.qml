//
//  WalletSetupLightbox.qml
//  qml/hifi/commerce/wallet
//
//  WalletSetupLightbox
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
    property string lastPage: "initialize";
    // Style
    color: hifi.colors.baseGray;

    Hifi.QmlCommerce {
        id: commerce;

        onSecurityImageResult: {
            if (!exists && root.lastPage === "securityImage") {
                // ERROR! Invalid security image.
                securityImageContainer.visible = true;
                choosePassphraseContainer.visible = false;
            }
        }

        onWalletAuthenticatedStatusResult: {
            securityImageContainer.visible = false;
            if (isAuthenticated) {
                privateKeysReadyContainer.visible = true;
            } else {
                choosePassphraseContainer.visible = true;
            }
        }

        onKeyFilePathIfExistsResult: {
            keyFilePath.text = path;
        }
    }

    //
    // SECURITY IMAGE SELECTION START
    //
    Item {
        id: securityImageContainer;
        // Anchors
        anchors.fill: parent;

        Item {
            id: securityImageTitle;
            // Size
            width: parent.width;
            height: 50;
            // Anchors
            anchors.left: parent.left;
            anchors.top: parent.top;

            // Title Bar text
            RalewaySemiBold {
                text: "WALLET SETUP - STEP 1 OF 3";
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
        }

        // Text below title bar
        RalewaySemiBold {
            id: securityImageTitleHelper;
            text: "Choose a Security Picture:";
            // Text size
            size: 24;
            // Anchors
            anchors.top: securityImageTitle.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            height: 50;
            width: paintedWidth;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        SecurityImageSelection {
            id: securityImageSelection;
            // Anchors
            anchors.top: securityImageTitleHelper.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: 280;

            Connections {
                onSendSignalToWallet: {
                    sendSignalToWallet(msg);
                }
            }
        }

        // Text below security images
        RalewayRegular {
            text: "<b>Your security picture shows you that the service asking for your passphrase is authorized.</b> You can change your secure picture at any time.";
            // Text size
            size: 18;
            // Anchors
            anchors.top: securityImageSelection.bottom;
            anchors.topMargin: 40;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Navigation Bar
        Item {
            // Size
            width: parent.width;
            height: 100;
            // Anchors:
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;

            // "Cancel" button
            HifiControlsUit.Button {
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: 100;
                text: "Cancel"
                onClicked: {
                    sendSignalToWallet({method: 'walletSetup_cancelClicked'});
                }
            }

            // "Next" button
            HifiControlsUit.Button {
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: 100;
                text: "Next";
                onClicked: {
                    root.lastPage = "securityImage";
                    var securityImagePath = securityImageSelection.getImagePathFromImageID(securityImageSelection.getSelectedImageIndex())
                    commerce.chooseSecurityImage(securityImagePath);
                    securityImageContainer.visible = false;
                    choosePassphraseContainer.visible = true;
                    passphraseSelection.clearPassphraseFields();
                }
            }
        }
    }
    //
    // SECURITY IMAGE SELECTION END
    //

    //
    // SECURE PASSPHRASE SELECTION START
    //
    Item {
        id: choosePassphraseContainer;
        visible: false;
        // Anchors
        anchors.fill: parent;

        onVisibleChanged: {
            if (visible) {
                commerce.getWalletAuthenticatedStatus();
            }
        }

        Item {
            id: passphraseTitle;
            // Size
            width: parent.width;
            height: 50;
            // Anchors
            anchors.left: parent.left;
            anchors.top: parent.top;

            // Title Bar text
            RalewaySemiBold {
                text: "WALLET SETUP - STEP 2 OF 3";
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
        }

        // Text below title bar
        RalewaySemiBold {
            id: passphraseTitleHelper;
            text: "Choose a Secure Passphrase";
            // Text size
            size: 24;
            // Anchors
            anchors.top: passphraseTitle.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: 50;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        PassphraseSelection {
            id: passphraseSelection;
            anchors.top: passphraseTitleHelper.bottom;
            anchors.topMargin: 30;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: passphraseNavBar.top;

            Connections {
                onSendMessageToLightbox: {
                    if (msg.method === 'statusResult') {
                    } else {
                        sendSignalToWallet(msg);
                    }
                }
            }
        }

        // Navigation Bar
        Item {
            id: passphraseNavBar;
            // Size
            width: parent.width;
            height: 100;
            // Anchors:
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;

            // "Back" button
            HifiControlsUit.Button {
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: 100;
                text: "Back"
                onClicked: {
                    root.lastPage = "choosePassphrase";
                    choosePassphraseContainer.visible = false;
                    securityImageContainer.visible = true;
                }
            }

            // "Next" button
            HifiControlsUit.Button {
                id: passphrasePageNextButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: 100;
                text: "Next";
                onClicked: {
                    if (passphraseSelection.validateAndSubmitPassphrase()) {
                        root.lastPage = "choosePassphrase";
                        commerce.generateKeyPair();
                        choosePassphraseContainer.visible = false;
                        privateKeysReadyContainer.visible = true;
                    }
                }
            }
        }
    }
    //
    // SECURE PASSPHRASE SELECTION END
    //

    //
    // PRIVATE KEYS READY START
    //
    Item {
        id: privateKeysReadyContainer;
        visible: false;
        // Anchors
        anchors.fill: parent;

        Item {
            id: keysReadyTitle;
            // Size
            width: parent.width;
            height: 50;
            // Anchors
            anchors.left: parent.left;
            anchors.top: parent.top;

            // Title Bar text
            RalewaySemiBold {
                text: "WALLET SETUP - STEP 3 OF 3";
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
        }

        // Text below title bar
        RalewaySemiBold {
            id: keysReadyTitleHelper;
            text: "Your Private Keys are Ready";
            // Text size
            size: 24;
            // Anchors
            anchors.top: keysReadyTitle.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: 50;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Text below checkbox
        RalewayRegular {
            id: explanationText;
            text: "Your money and purchases are secured with private keys that only you have access to. " +
            "<b>If they are lost, you will not be able to access your money or purchases.</b><br><br>" +
            "<b>To protect your privacy, High Fidelity has no access to your private keys and cannot " +
            "recover them for any reason.<br><br>To safeguard your private keys, backup this file on a regular basis:</b>";
            // Text size
            size: 16;
            // Anchors
            anchors.top: keysReadyTitleHelper.bottom;
            anchors.topMargin: 16;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        HifiControlsUit.TextField {
            id: keyFilePath;
            anchors.top: explanationText.bottom;
            anchors.topMargin: 10;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: clipboardButton.left;
            height: 40;
            readOnly: true;

            onVisibleChanged: {
                if (visible) {
                    commerce.getKeyFilePathIfExists();
                }
            }
        }
        HifiControlsUit.Button {
            id: clipboardButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            anchors.top: keyFilePath.top;
            anchors.bottom: keyFilePath.bottom;
            width: height;
            HiFiGlyphs {
                text: hifi.glyphs.question;
                // Size
                size: parent.height*1.3;
                // Anchors
                anchors.fill: parent;
                // Style
                horizontalAlignment: Text.AlignHCenter;
                color: enabled ? hifi.colors.white : hifi.colors.faintGray;
            }

            onClicked: {
                Window.copyToClipboard(keyFilePath.text);
            }
        }

        // Navigation Bar
        Item {
            // Size
            width: parent.width;
            height: 100;
            // Anchors:
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            // "Next" button
            HifiControlsUit.Button {
                id: keysReadyPageNextButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: 100;
                text: "Finish";
                onClicked: {
                    root.visible = false;
                    sendSignalToWallet({method: 'walletSetup_finished'});
                }
            }
        }
    }
    //
    // PRIVATE KEYS READY END
    //

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendSignalToWallet(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
