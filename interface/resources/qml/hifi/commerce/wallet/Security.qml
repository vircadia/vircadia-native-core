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
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    property string keyFilePath;

    Connections {
        target: Commerce;

        onKeyFilePathIfExistsResult: {
            root.keyFilePath = path;
        }
    }

    // Username Text
    RalewayRegular {
        id: usernameText;
        text: Account.username;
        // Text size
        size: 24;
        // Style
        color: hifi.colors.white;
        elide: Text.ElideRight;
        // Anchors
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        width: parent.width/2;
        height: 80;
    }

    Item {
        id: securityContainer;
        anchors.top: usernameText.bottom;
        anchors.topMargin: 20;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        RalewaySemiBold {
            id: securityText;
            text: "Security";
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            height: 30;
            // Text size
            size: 18;
            // Style
            color: hifi.colors.blueHighlight;
        }

        Rectangle {
            id: securityTextSeparator;
            // Size
            width: parent.width;
            height: 1;
            // Anchors
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: securityText.bottom;
            anchors.topMargin: 8;
            // Style
            color: hifi.colors.faintGray;
        }

        Item {
            id: changePassphraseContainer;
            anchors.top: securityTextSeparator.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.leftMargin: 40;
            anchors.right: parent.right;
            anchors.rightMargin: 55;
            height: 75;

            HiFiGlyphs {
                id: changePassphraseImage;
                text: hifi.glyphs.passphrase;
                // Size
                size: 80;
                // Anchors
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                // Style
                color: hifi.colors.white;
            }

            RalewaySemiBold {
                text: "Passphrase";
                // Anchors
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: changePassphraseImage.right;
                anchors.leftMargin: 30;
                width: 50;
                // Text size
                size: 18;
                // Style
                color: hifi.colors.white;
            }

            // "Change Passphrase" button
            HifiControlsUit.Button {
                id: changePassphraseButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: parent.right;
                anchors.verticalCenter: parent.verticalCenter;
                width: 140;
                height: 40;
                text: "Change";
                onClicked: {
                    sendSignalToWallet({method: 'walletSecurity_changePassphrase'});
                }
            }
        }

        Rectangle {
            id: changePassphraseSeparator;
            // Size
            width: parent.width;
            height: 1;
            // Anchors
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: changePassphraseContainer.bottom;
            anchors.topMargin: 8;
            // Style
            color: hifi.colors.faintGray;
        }

        Item {
            id: changeSecurityImageContainer;
            anchors.top: changePassphraseSeparator.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.leftMargin: 40;
            anchors.right: parent.right;
            anchors.rightMargin: 55;
            height: 75;

            HiFiGlyphs {
                id: changeSecurityImageImage;
                text: hifi.glyphs.securityImage;
                // Size
                size: 80;
                // Anchors
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                // Style
                color: hifi.colors.white;
            }

            RalewaySemiBold {
                text: "Security Pic";
                // Anchors
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: changeSecurityImageImage.right;
                anchors.leftMargin: 30;
                width: 50;
                // Text size
                size: 18;
                // Style
                color: hifi.colors.white;
            }

            // "Change Passphrase" button
            HifiControlsUit.Button {
                id: changeSecurityImageButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: parent.right;
                anchors.verticalCenter: parent.verticalCenter;
                width: 140;
                height: 40;
                text: "Change";
                onClicked: {
                    sendSignalToWallet({method: 'walletSecurity_changeSecurityImage'});
                }
            }
        }

        Rectangle {
            id: privateKeysSeparator;
            // Size
            width: parent.width;
            height: 1;
            // Anchors
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: changeSecurityImageContainer.bottom;
            anchors.topMargin: 8;
            // Style
            color: hifi.colors.faintGray;
        }

        Item {
            id: yourPrivateKeysContainer;
            anchors.top: privateKeysSeparator.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 40;
            anchors.right: parent.right;
            anchors.rightMargin: 55;
            anchors.bottom: parent.bottom;

            onVisibleChanged: {
                if (visible) {
                    Commerce.getKeyFilePathIfExists();
                }
            }

            HiFiGlyphs {
                id: yourPrivateKeysImage;
                text: hifi.glyphs.walletKey;
                // Size
                size: 80;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 20;
                anchors.left: parent.left;
                // Style
                color: hifi.colors.white;
            }

            RalewaySemiBold {
                id: yourPrivateKeysText;
                text: "Private Keys";
                size: 18;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 32;
                anchors.left: yourPrivateKeysImage.right;
                anchors.leftMargin: 30;
                anchors.right: parent.right;
                height: 30;
                // Style
                color: hifi.colors.white;
            }

            // Text below "private keys"
            RalewayRegular {
                id: explanitoryText;
                text: "Your money and purchases are secured with private keys that only you have access to.";
                // Text size
                size: 18;
                // Anchors
                anchors.top: yourPrivateKeysText.bottom;
                anchors.topMargin: 10;
                anchors.left: yourPrivateKeysText.left;
                anchors.right: yourPrivateKeysText.right;
                height: paintedHeight;
                // Style
                color: hifi.colors.white;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignVCenter;
            }

            Rectangle {
                id: removeHmdContainer;
                z: 998;
                visible: false;

                gradient: Gradient {
                    GradientStop {
                        position: 0.2;
                        color: hifi.colors.baseGrayHighlight;
                    }
                    GradientStop {
                        position: 1.0;
                        color: hifi.colors.baseGrayShadow;
                    }
                }
                anchors.fill: backupInstructionsButton;
                radius: 5;
                MouseArea {
                    anchors.fill: parent;
                    propagateComposedEvents: false;
                }

                RalewayBold {
                    anchors.fill: parent;
                    text: "INSTRUCTIONS OPEN ON DESKTOP";
                    size: 15;
                    color: hifi.colors.white;
                    verticalAlignment: Text.AlignVCenter;
                    horizontalAlignment: Text.AlignHCenter;
                }

                    Timer {
                        id: removeHmdContainerTimer;
                        interval: 5000;
                        onTriggered: removeHmdContainer.visible = false
                    }
            }

            HifiControlsUit.Button {
                id: backupInstructionsButton;
                text: "View Backup Instructions";
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.left: explanitoryText.left;
                anchors.right: explanitoryText.right;
                anchors.top: explanitoryText.bottom;
                anchors.topMargin: 16;
                height: 40;

                onClicked: {
                    var keyPath = "file:///" + root.keyFilePath.substring(0, root.keyFilePath.lastIndexOf('/'));
                    Qt.openUrlExternally(keyPath + "/backup_instructions.html");
                    Qt.openUrlExternally(keyPath);
                    removeHmdContainer.visible = true;
                    removeHmdContainerTimer.start();
                }
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
            default:
                console.log('Unrecognized message from wallet.js:', JSON.stringify(message));
        }
    }
    signal sendSignalToWallet(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
