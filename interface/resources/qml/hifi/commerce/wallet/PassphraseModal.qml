//
//  PassphraseModal.qml
//  qml/hifi/commerce/wallet
//
//  PassphraseModal
//
//  Created by Zach Fox on 2017-08-18
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

Item {
    HifiConstants { id: hifi; }

    id: root;
    Hifi.QmlCommerce {
        id: commerce;

        onSecurityImageResult: {
            passphraseModalSecurityImage.source = "";
            passphraseModalSecurityImage.source = "image://security/securityImage";
        }
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
    }
    
    // This will cause a bug -- if you bring up passphrase selection in HUD mode while
    // in HMD while having HMD preview enabled, then move, then finish passphrase selection,
    // HMD preview will stay off.
    // TODO: Fix this unlikely bug
    onVisibleChanged: {
        if (visible) {
            passphraseField.focus = true;
            sendSignalToWallet({method: 'disableHmdPreview'});
        } else {
            sendSignalToWallet({method: 'maybeEnableHmdPreview'});
        }
    }

    // Background rectangle
    Rectangle {
        anchors.fill: parent;
        color: "black";
        opacity: 0.9;
    }

    Rectangle {
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: 220;
        color: hifi.colors.baseGray;

        RalewaySemiBold {
            id: instructionsText;
            text: "Enter Wallet Passphrase";
            size: 16;
            anchors.top: parent.top;
            anchors.topMargin: 30;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            width: passphraseField.width;
            height: paintedHeight;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignLeft;
        }

        HifiControlsUit.TextField {
            id: passphraseField;
            anchors.top: instructionsText.bottom;
            anchors.topMargin: 4;
            anchors.left: instructionsText.left;
            width: 280;
            height: 50;
            echoMode: TextInput.Password;
            placeholderText: "passphrase";

            onFocusChanged: {
                if (focus) {
                    sendSignalToWallet({method: 'walletSetup_raiseKeyboard'});
                } else if (!passphraseFieldAgain.focus) {
                    sendSignalToWallet({method: 'walletSetup_lowerKeyboard'});
                }
            }

            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    parent.focus = true;
                    sendSignalToWallet({method: 'walletSetup_raiseKeyboard'});
                }
            }

            onAccepted: {
                //commerce.submitWalletPassphrase(passphraseField.text);
            }
        }

        // Show passphrase text
        HifiControlsUit.CheckBox {
            id: showPassphrase;
            colorScheme: hifi.colorSchemes.dark;
            anchors.left: passphraseField.left;
            anchors.top: passphraseField.bottom;
            anchors.topMargin: 8;
            height: 30;
            text: "Show passphrase as plain text";
            boxSize: 24;
            onClicked: {
                passphraseField.echoMode = checked ? TextInput.Normal : TextInput.Password;
            }
        }        

        // Security Image
        Item {
            id: securityImageContainer;
            // Anchors
            anchors.top: instructionsText.top;
            anchors.left: passphraseField.right;
            anchors.leftMargin: 12;
            anchors.right: parent.right;
            Image {
                id: passphraseModalSecurityImage;
                anchors.top: parent.top;
                anchors.horizontalCenter: parent.horizontalCenter;
                height: 75;
                width: height;
                fillMode: Image.PreserveAspectFit;
                mipmap: true;
                source: "image://security/securityImage";
                cache: false;
                onVisibleChanged: {
                    commerce.getSecurityImage();
                }
            }
            Image {
                id: passphraseModalSecurityImageOverlay;
                source: "images/lockOverlay.png";
                width: passphraseModalSecurityImage.width * 0.45;
                height: passphraseModalSecurityImage.height * 0.45;
                anchors.bottom: passphraseModalSecurityImage.bottom;
                anchors.right: passphraseModalSecurityImage.right;
                mipmap: true;
                opacity: 0.9;
            }
            // "Security image" text below pic
            RalewayRegular {
                text: "security image";
                // Text size
                size: 12;
                // Anchors
                anchors.top: passphraseModalSecurityImage.bottom;
                anchors.topMargin: 4;
                anchors.left: securityImageContainer.left;
                anchors.right: securityImageContainer.right;
                height: paintedHeight;
                // Style
                color: hifi.colors.faintGray;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
        }
        
        //
        // ACTION BUTTONS START
        //
        Item {
            id: passphrasePopupActionButtonsContainer;
            // Size
            width: root.width;
            height: 50;
            // Anchors
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 10;

            // "Cancel" button
            HifiControlsUit.Button {
                id: cancelPassphraseInputButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                height: 40;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: parent.width/2 - anchors.leftMargin*2;
                text: "Cancel"
                onClicked: {
                    sendSignalToWallet({method: 'passphrasePopup_cancelClicked'});
                }
            }

            // "Submit" button
            HifiControlsUit.Button {
                id: submitPassphraseInputButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                height: 40;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: parent.width/2 - anchors.rightMargin*2;
                text: "Submit"
                onClicked: {
                    //commerce.submitWalletPassphrase(passphraseField.text);
                }
            }
        }
    }

    signal sendSignalToWallet(var msg);
}
