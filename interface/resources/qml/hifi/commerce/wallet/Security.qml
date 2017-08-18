//
//  Security.qml
//  qml/hifi/commerce/wallet
//
//  Security
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
    
    // Security Image
    Image {
        id: topSecurityImage;
        // Anchors
        anchors.top: parent.top;
        anchors.left: parent.left;
        height: 65;
        width: height;
        fillMode: Image.PreserveAspectFit;
        mipmap: true;
    }

    // Username Text
    RalewayRegular {
        id: usernameText;
        text: Account.username;
        // Text size
        size: 24;
        // Style
        color: hifi.colors.faintGray;
        elide: Text.ElideRight;
        // Anchors
        anchors.top: topSecurityImage.top;
        anchors.bottom: topSecurityImage.bottom;
        anchors.left: topSecurityImage.right;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
    }

    Item {
        id: securityContainer;
        anchors.top: topSecurityImage.bottom;
        anchors.topMargin: 20;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: childrenRect.height;

        RalewayRegular {
            id: securityText;
            text: "Security";
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 30;
            // Text size
            size: 22;
            // Style
            color: hifi.colors.faintGray;
        }

        Item {
            id: resetPassphraseContainer;
            anchors.top: securityText.bottom;
            anchors.topMargin: 16;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 75;

            Image {
                id: resetPassphraseImage;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                height: parent.height;
                width: height;
                fillMode: Image.PreserveAspectFit;
                mipmap: true;
            }
            // "Reset Passphrase" button
            HifiControlsUit.Button {
                id: resetPassphraseButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.left: resetPassphraseImage.right;
                anchors.leftMargin: 16;
                width: 250;
                height: 50;
                text: "Reset My Passphrase";
                onClicked: {
                    
                }
            }
        }

        Item {
            id: resetSecurityImageContainer;
            anchors.top: resetPassphraseContainer.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 75;

            Image {
                id: resetSecurityImageImage;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                height: parent.height;
                width: height;
                fillMode: Image.PreserveAspectFit;
                mipmap: true;
            }
            // "Reset Security Image" button
            HifiControlsUit.Button {
                id: resetSecurityImageButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.left: resetSecurityImageImage.right;
                anchors.leftMargin: 16;
                width: 250;
                height: 50;
                text: "Reset My Security Image";
                onClicked: {
                    
                }
            }
        }
    }

    Item {
        id: yourPrivateKeysContainer;
        anchors.top: securityContainer.bottom;
        anchors.topMargin: 20;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: childrenRect.height;

        RalewaySemiBold {
            id: yourPrivateKeysText;
            text: "Your Private Keys";
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 30;
            // Text size
            size: 22;
            // Style
            color: hifi.colors.faintGray;
        }

        // Text below "your private keys"
        RalewayRegular {
            text: "Your money and purchases are secured with private keys that only you " +
            "have access to. <b>If they are lost, you will not be able to access your money or purchases.</b> " +
            "To ensure they are not lost, it is imperative that you back up your private keys.<br><br>" +
            "<b>To safeguard your private keys, back up this file regularly:</b>";
            // Text size
            size: 18;
            // Anchors
            anchors.top: yourPrivateKeysText.bottom;
            anchors.topMargin: 20;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter;
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    function setSecurityImages(imagePath) {
        topSecurityImage.source = imagePath;
        resetSecurityImageImage.source = imagePath;
    }
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
    signal sendSignalToWallet(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
