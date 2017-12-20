//
//  PassphraseSelection.qml
//  qml/hifi/commerce/wallet
//
//  PassphraseSelection
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
    property bool isChangingPassphrase: false;
    property bool isShowingTip: false;
    property bool shouldImmediatelyFocus: true;

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
    }

    Connections {
        target: Commerce;
        onSecurityImageResult: {
            passphrasePageSecurityImage.source = "";
            passphrasePageSecurityImage.source = "image://security/securityImage";
        }

        onChangePassphraseStatusResult: {
            sendMessageToLightbox({method: 'statusResult', status: changeSuccess});
        }
    }

    // This will cause a bug -- if you bring up passphrase selection in HUD mode while
    // in HMD while having HMD preview enabled, then move, then finish passphrase selection,
    // HMD preview will stay off.
    // TODO: Fix this unlikely bug
    onVisibleChanged: {
        if (visible) {
            if (root.shouldImmediatelyFocus) {
                focusFirstTextField();
            }
            sendMessageToLightbox({method: 'disableHmdPreview'});
        } else {
            sendMessageToLightbox({method: 'maybeEnableHmdPreview'});
        }
    }
    

    HifiControlsUit.TextField {
        id: currentPassphraseField;
        colorScheme: hifi.colorSchemes.dark;
        visible: root.isChangingPassphrase;
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        anchors.right: passphraseField.right;
        height: 50;
        echoMode: TextInput.Password;
        placeholderText: "enter current passphrase";
        activeFocusOnPress: true;
        activeFocusOnTab: true;

        onFocusChanged: {
            if (focus) {
                var hidePassword = (currentPassphraseField.echoMode === TextInput.Password);
                sendSignalToWallet({method: 'walletSetup_raiseKeyboard', isPasswordField: hidePassword});
            }
        }

        onAccepted: {
            passphraseField.focus = true;
        }
    }

    HifiControlsUit.TextField {
        id: passphraseField;
        colorScheme: hifi.colorSchemes.dark;
        anchors.top: root.isChangingPassphrase ? currentPassphraseField.bottom : parent.top;
        anchors.topMargin: root.isChangingPassphrase ? 40 : 0;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        width: 285;
        height: 50;
        echoMode: TextInput.Password;
        placeholderText: root.isShowingTip ? "" : "enter new passphrase";
        activeFocusOnPress: true;
        activeFocusOnTab: true;

        onFocusChanged: {
            if (focus) {
                var hidePassword = (passphraseField.echoMode === TextInput.Password);
                sendMessageToLightbox({method: 'walletSetup_raiseKeyboard', isPasswordField: hidePassword});
            }
        }

        onAccepted: {
            passphraseFieldAgain.focus = true;
        }
    }

    HifiControlsUit.TextField {
        id: passphraseFieldAgain;
        colorScheme: hifi.colorSchemes.dark;
        anchors.top: passphraseField.bottom;
        anchors.topMargin: root.isChangingPassphrase ? 20 : 40;
        anchors.left: passphraseField.left;
        anchors.right: passphraseField.right;
        height: 50;
        echoMode: TextInput.Password;
        placeholderText: root.isShowingTip ? "" : "re-enter new passphrase";
        activeFocusOnPress: true;
        activeFocusOnTab: true;

        onFocusChanged: {
            if (focus) {
                var hidePassword = (passphraseFieldAgain.echoMode === TextInput.Password);
                sendMessageToLightbox({method: 'walletSetup_raiseKeyboard', isPasswordField: hidePassword});
            }
        }

        onAccepted: {
            focus = false;
        }
    }

    // Security Image
    Item {
        id: securityImageContainer;
        // Anchors
        anchors.top: root.isChangingPassphrase ? currentPassphraseField.top : passphraseField.top;
        anchors.left: passphraseField.right;
        anchors.right: parent.right;
        anchors.bottom: root.isChangingPassphrase ? passphraseField.bottom : passphraseFieldAgain.bottom;
        Image {
            id: passphrasePageSecurityImage;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: iconAndTextContainer.top;
            fillMode: Image.PreserveAspectFit;
            mipmap: true;
            source: "image://security/securityImage";
            cache: false;
            onVisibleChanged: {
                Commerce.getSecurityImage();
            }
        }
        Item {
            id: iconAndTextContainer;
            anchors.left: passphrasePageSecurityImage.left;
            anchors.right: passphrasePageSecurityImage.right;
            anchors.bottom: parent.bottom;
            height: 22;
            // Lock icon
            HiFiGlyphs {
                id: lockIcon;
                text: hifi.glyphs.lock;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                anchors.leftMargin: 35;
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

    // Error text above TextFields
    RalewaySemiBold {
        id: errorText;
        text: "";
        // Text size
        size: 15;
        // Anchors
        anchors.bottom: passphraseField.top;
        anchors.bottomMargin: 4;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        anchors.right: securityImageContainer.left;
        anchors.rightMargin: 4;
        height: 30;
        // Style
        color: hifi.colors.redHighlight;
        // Alignment
        verticalAlignment: Text.AlignVCenter;
    }

    // Show passphrase text
    HifiControlsUit.CheckBox {
        id: showPassphrase;
        visible: !root.isShowingTip;
        colorScheme: hifi.colorSchemes.dark;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        anchors.top: passphraseFieldAgain.bottom;
        anchors.topMargin: 16;
        height: 30;
        text: "Show passphrase";
        boxSize: 24;
        onClicked: {
            passphraseField.echoMode = checked ? TextInput.Normal : TextInput.Password;
            passphraseFieldAgain.echoMode = checked ? TextInput.Normal : TextInput.Password;
            if (root.isChangingPassphrase) {
                currentPassphraseField.echoMode = checked ? TextInput.Normal : TextInput.Password;
            }
        }
    }

    // Text below checkbox
    RalewayRegular {
        visible: !root.isShowingTip;
        text: "Your passphrase is used to encrypt your private keys. Only you have it.<br><br>Please write it down.<br><br><b>If it is lost, you will not be able to recover it.</b>";
        // Text size
        size: 18;
        // Anchors
        anchors.top: showPassphrase.bottom;
        anchors.topMargin: 16;
        anchors.left: parent.left;
        anchors.leftMargin: 24;
        anchors.right: parent.right;
        anchors.rightMargin: 24;
        height: paintedHeight;
        // Style
        color: hifi.colors.white;
        wrapMode: Text.WordWrap;
        // Alignment
        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;
    }

    function validateAndSubmitPassphrase() {
        if (passphraseField.text.length < 3) {
            setErrorText("Passphrase must be at least 3 characters.");
            passphraseField.error = true;
            passphraseFieldAgain.error = true;
            currentPassphraseField.error = true;
            return false;
        } else if (passphraseField.text !== passphraseFieldAgain.text) {
            setErrorText("Passphrases don't match.");
            passphraseField.error = true;
            passphraseFieldAgain.error = true;
            currentPassphraseField.error = true;
            return false;
        } else {
            passphraseField.error = false;
            passphraseFieldAgain.error = false;
            currentPassphraseField.error = false;
            setErrorText("");
            Commerce.changePassphrase(currentPassphraseField.text, passphraseField.text);
            return true;
        }
    }

    function setErrorText(text) {
        errorText.text = text;
    }

    function clearPassphraseFields() {
        currentPassphraseField.text = "";
        passphraseField.text = "";
        passphraseFieldAgain.text = "";
        setErrorText("");
    }

    function focusFirstTextField() {
        if (root.isChangingPassphrase) {
            currentPassphraseField.focus = true;
        } else {
            passphraseField.focus = true;
        }
    }

    signal sendMessageToLightbox(var msg);
}
