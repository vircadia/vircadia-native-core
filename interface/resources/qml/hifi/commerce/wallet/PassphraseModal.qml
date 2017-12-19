//
//  PassphraseModal.qml
//  qml/hifi/commerce/wallet
//
//  PassphraseModal
//
//  Created by Zach Fox on 2017-08-31
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
import "../common" as HifiCommerceCommon

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    z: 997;
    property bool keyboardRaised: false;
    property bool isPasswordField: false;
    property string titleBarIcon: "";
    property string titleBarText: "";

    Image {
        anchors.fill: parent;
        source: "images/wallet-bg.jpg";
    }

    Hifi.QmlCommerce {
        id: commerce;

        onSecurityImageResult: {
            titleBarSecurityImage.source = "";
            titleBarSecurityImage.source = "image://security/securityImage";
            passphraseModalSecurityImage.source = "";
            passphraseModalSecurityImage.source = "image://security/securityImage";
        }

        onWalletAuthenticatedStatusResult: {
            submitPassphraseInputButton.enabled = true;
            if (!isAuthenticated) {
                errorText.text = "Authentication failed - please try again.";
                UserActivityLogger.commercePassphraseAuthenticationStatus("auth failure");
            } else {
                sendSignalToParent({method: 'authSuccess'});
                UserActivityLogger.commercePassphraseAuthenticationStatus("auth success");
            }
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
            sendSignalToParent({method: 'disableHmdPreview'});
        } else {
            sendSignalToParent({method: 'maybeEnableHmdPreview'});
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;
    }

    Item {
        id: titleBar;
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: 50;

        // Wallet icon
        HiFiGlyphs {
            id: titleBarIcon;
            text: root.titleBarIcon;
            // Size
            size: parent.height * 0.8;
            // Anchors
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.verticalCenter: parent.verticalCenter;
            // Style
            color: hifi.colors.blueHighlight;
        }

        RalewaySemiBold {
            id: titleBarText;
            text: root.titleBarText;
            anchors.top: parent.top;
            anchors.left: titleBarIcon.right;
            anchors.leftMargin: 4;
            anchors.bottom: parent.bottom;
            anchors.right: parent.right;
            size: 20;
            color: hifi.colors.white;
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
            fillMode: Image.PreserveAspectFit;
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

    Item {
        id: passphraseContainer;
        anchors.top: titleBar.bottom;
        anchors.left: parent.left;
        anchors.leftMargin: 8;
        anchors.right: parent.right;
        anchors.rightMargin: 8;
        height: 250;

        RalewaySemiBold {
            id: instructionsText;
            text: "Please Enter Your Passphrase";
            size: 24;
            anchors.top: parent.top;
            anchors.topMargin: 30;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            width: passphraseField.width;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
            // Alignment
            horizontalAlignment: Text.AlignLeft;
        }

        // Error text above buttons
        RalewaySemiBold {
            id: errorText;
            text: "";
            // Text size
            size: 15;
            // Anchors
            anchors.bottom: passphraseField.top;
            anchors.bottomMargin: 4;
            anchors.left: passphraseField.left;
            anchors.right: parent.right;
            height: 20;
            // Style
            color: hifi.colors.redHighlight;
        }

        HifiControlsUit.TextField {
            id: passphraseField;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: instructionsText.bottom;
            anchors.topMargin: 40;
            anchors.left: instructionsText.left;
            width: 260;
            height: 50;
            echoMode: TextInput.Password;
            placeholderText: "passphrase";
            activeFocusOnPress: true;
            activeFocusOnTab: true;

            onFocusChanged: {
                root.keyboardRaised = focus;
                root.isPasswordField = (focus && passphraseField.echoMode === TextInput.Password);
            }

            onAccepted: {
                submitPassphraseInputButton.enabled = false;
                commerce.setPassphrase(passphraseField.text);
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
            text: "Show passphrase";
            boxSize: 24;
            onClicked: {
                passphraseField.echoMode = checked ? TextInput.Normal : TextInput.Password;
            }
        }

        // Security Image
        Item {
            id: securityImageContainer;
            // Anchors
            anchors.top: passphraseField.top;
            anchors.left: passphraseField.right;
            anchors.leftMargin: 8;
            anchors.right: parent.right;
            anchors.rightMargin: 8;
            height: 145;
            Image {
                id: passphraseModalSecurityImage;
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.bottom: iconAndTextContainer.top;
                fillMode: Image.PreserveAspectFit;
                mipmap: true;
                source: "image://security/securityImage";
                cache: false;
                onVisibleChanged: {
                    commerce.getSecurityImage();
                }
            }
            Item {
                id: iconAndTextContainer;
                anchors.left: passphraseModalSecurityImage.left;
                anchors.right: passphraseModalSecurityImage.right;
                anchors.bottom: parent.bottom;
                height: 24;
                // Lock icon
                HiFiGlyphs {
                    id: lockIcon;
                    text: hifi.glyphs.lock;
                    anchors.bottom: parent.bottom;
                    anchors.left: parent.left;
                    anchors.leftMargin: 30;
                    size: 20;
                    width: height;
                    verticalAlignment: Text.AlignBottom;
                    color: hifi.colors.white;
                }
                // "Security image" text below pic
                RalewayRegular {
                    id: securityImageText;
                    text: "SECURITY PIC";
                    // Text size
                    size: 12;
                    // Anchors
                    anchors.bottom: parent.bottom;
                    anchors.right: parent.right;
                    anchors.rightMargin: lockIcon.anchors.leftMargin;
                    width: paintedWidth;
                    height: 22;
                    // Style
                    color: hifi.colors.white;
                    // Alignment
                    horizontalAlignment: Text.AlignRight;
                    verticalAlignment: Text.AlignBottom;
                }
            }
        }
        
        //
        // ACTION BUTTONS START
        //
        Item {
            id: passphrasePopupActionButtonsContainer;
            // Anchors
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;

            // "Submit" button
            HifiControlsUit.Button {
                id: submitPassphraseInputButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 20;
                height: 40;
                anchors.left: parent.left;
                anchors.leftMargin: 16;
                anchors.right: parent.right;
                anchors.rightMargin: 16;
                width: parent.width/2 -4;
                text: "Submit"
                onClicked: {
                    submitPassphraseInputButton.enabled = false;
                    commerce.setPassphrase(passphraseField.text);
                }
            }

            // "Cancel" button
            HifiControlsUit.Button {
                id: cancelPassphraseInputButton;
                color: hifi.buttons.noneBorderlessWhite;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: submitPassphraseInputButton.bottom;
                anchors.topMargin: 20;
                height: 40;
                anchors.left: parent.left;
                anchors.leftMargin: 16;
                anchors.right: parent.right;
                anchors.rightMargin: 16;
                width: parent.width/2 - 4;
                text: "Cancel"
                onClicked: {
                    sendSignalToParent({method: 'passphrasePopup_cancelClicked'});
                    UserActivityLogger.commercePassphraseAuthenticationStatus("passphrase modal cancelled");
                }
            }
        }
    }

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
            password: root.isPasswordField;
            anchors {
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
            }
        }
    }

    signal sendSignalToParent(var msg);
}
