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
            id: changeSecurityImageContainer;
            anchors.top: securityTextSeparator.bottom;
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

            // "Change Security Pic" button
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

        Item {
            id: autoLogoutContainer;
            anchors.top: changeSecurityImageContainer.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.leftMargin: 40;
            anchors.right: parent.right;
            anchors.rightMargin: 55;
            height: 75;

            HiFiGlyphs {	
                id: autoLogoutImage;	
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

            HifiControlsUit.CheckBox {
                id: autoLogoutCheckbox;
                checked: Settings.getValue("wallet/autoLogout", false);
                text: "Automatically Log Out when Exiting Interface"
                // Anchors
                anchors.verticalCenter: autoLogoutImage.verticalCenter;
                anchors.left: autoLogoutImage.right;
                anchors.leftMargin: 20;
                anchors.right: autoLogoutHelp.left;
                anchors.rightMargin: 12;
                boxSize: 28;
                labelFontSize: 18;
                color: hifi.colors.white;
                onCheckedChanged: {
                    Settings.setValue("wallet/autoLogout", checked);
                    if (checked) {
                        Settings.setValue("wallet/savedUsername", Account.username);
                    } else {
                        Settings.setValue("wallet/savedUsername", "");
                    }
                }
            }

            RalewaySemiBold {
                id: autoLogoutHelp;
                text: '[?]';
                // Anchors
                anchors.verticalCenter: autoLogoutImage.verticalCenter;
                anchors.right: parent.right;
                width: 30;
                height: 30;
                // Text size
                size: 18;
                // Style
                color: hifi.colors.blueHighlight;

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onEntered: {
                        parent.color = hifi.colors.blueAccent;
                    }
                    onExited: {
                        parent.color = hifi.colors.blueHighlight;
                    }
                    onClicked: {
                        sendSignalToWallet({method: 'walletSecurity_autoLogoutHelp'});
                    }
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
