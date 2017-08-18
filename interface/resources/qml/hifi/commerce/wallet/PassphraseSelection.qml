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

    Hifi.QmlCommerce {
        id: commerce;
    }

    onVisibleChanged: {
        if (visible) {
            passphraseField.focus = true;
        }
    }

    HifiControlsUit.TextField {
        id: passphraseField;
        anchors.top: parent.top;
        anchors.topMargin: 30;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        width: 280;
        height: 50;
        echoMode: TextInput.Password;
        placeholderText: "passphrase";
    }
    HifiControlsUit.TextField {
        id: passphraseFieldAgain;
        anchors.top: passphraseField.bottom;
        anchors.topMargin: 10;
        anchors.left: passphraseField.left;
        anchors.right: passphraseField.right;
        height: 50;
        echoMode: TextInput.Password;
        placeholderText: "re-enter passphrase";
    }

    // Security Image
    Item {
        id: securityImageContainer;
        // Anchors
        anchors.top: passphraseField.top;
        anchors.left: passphraseField.right;
        anchors.leftMargin: 12;
        anchors.right: parent.right;
        Image {
            id: passphrasePageSecurityImage;
            anchors.top: parent.top;
            anchors.horizontalCenter: parent.horizontalCenter;
            height: 75;
            width: height;
            fillMode: Image.PreserveAspectFit;
            mipmap: true;
        }
        // "Security picture" text below pic
        RalewayRegular {
            text: "security picture";
            // Text size
            size: 12;
            // Anchors
            anchors.top: passphrasePageSecurityImage.bottom;
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

    // Error text below TextFields
    RalewaySemiBold {
        id: errorText;
        text: "";
        // Text size
        size: 16;
        // Anchors
        anchors.top: passphraseFieldAgain.bottom;
        anchors.topMargin: 0;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
        height: 30;
        // Style
        color: hifi.colors.redHighlight;
        // Alignment
        horizontalAlignment: Text.AlignHLeft;
        verticalAlignment: Text.AlignVCenter;
    }

    // Text below TextFields
    RalewaySemiBold {
        id: passwordReqs;
        text: "Passphrase must be at least 4 characters";
        // Text size
        size: 16;
        // Anchors
        anchors.top: passphraseFieldAgain.bottom;
        anchors.topMargin: 16;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
        height: 30;
        // Style
        color: hifi.colors.faintGray;
        // Alignment
        horizontalAlignment: Text.AlignHLeft;
        verticalAlignment: Text.AlignVCenter;
    }

    // Show passphrase text
    HifiControlsUit.CheckBox {
        id: showPassphrase;
        colorScheme: hifi.colorSchemes.dark;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.top: passwordReqs.bottom;
        anchors.topMargin: 16;
        height: 30;
        text: "Show passphrase as plain text";
        boxSize: 24;
        onClicked: {
            passphraseField.echoMode = checked ? TextInput.Normal : TextInput.Password;
            passphraseFieldAgain.echoMode = checked ? TextInput.Normal : TextInput.Password;
        }
    }

    // Text below checkbox
    RalewayRegular {
        text: "Your passphrase is used to encrypt your private keys. <b>Please write it down.</b> If it is lost, you will not be able to recover it.";
        // Text size
        size: 16;
        // Anchors
        anchors.top: showPassphrase.bottom;
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
        horizontalAlignment: Text.AlignLeft;
        verticalAlignment: Text.AlignVCenter;
    }

    function validateAndSubmitPassphrase() {
        if (passphraseField.text.length < 4) {
            errorText.text = "Passphrase too short."
            return false;
        } else if (passphraseField.text !== passphraseFieldAgain.text) {
            errorText.text = "Passphrases don't match."
            return false;
        } else {
            errorText.text = "";
            commerce.setPassphrase(passphraseField.text);
            return true;
        }
    }

    function setSecurityImage(imageSource) {
        passphrasePageSecurityImage.source = imageSource;
    }
}
