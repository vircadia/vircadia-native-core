//
//  SecurityImageChange.qml
//  qml/hifi/commerce/wallet
//
//  SecurityImageChange
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
    property bool justSubmitted: false;

    Connections {
        target: Commerce;
        
        onSecurityImageResult: {
            securityImageChangePageSecurityImage.source = "";
            securityImageChangePageSecurityImage.source = "image://security/securityImage";
            if (exists) { // Success submitting new security image
                if (root.justSubmitted) {
                    sendSignalToWallet({method: "walletSecurity_changeSecurityImageSuccess"});
                    root.justSubmitted = false;
                }
            } else if (root.justSubmitted) {
                // Error submitting new security image.
                root.justSubmitted = false;
            }
        }
    }
    

    // Security Image
    Item {
        // Anchors
        anchors.top: parent.top;
        anchors.right: parent.right;
        anchors.rightMargin: 25;
        width: 130;
        height: 150;
        Image {
            id: securityImageChangePageSecurityImage;
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
            anchors.left: securityImageChangePageSecurityImage.left;
            anchors.right: securityImageChangePageSecurityImage.right;
            anchors.bottom: parent.bottom;
            height: 22;
            // Lock icon
            HiFiGlyphs {
                id: lockIcon;
                text: hifi.glyphs.lock;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                anchors.leftMargin: 10;
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
    // SECURITY IMAGE SELECTION START
    //
    Item {
        // Anchors
        anchors.top: parent.top;
        anchors.topMargin: 135;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        // "Change Security Image" text
        RalewaySemiBold {
            id: securityImageTitle;
            text: "Change Security Pic:";
            // Text size
            size: 18;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            height: 30;
            // Style
            color: hifi.colors.blueHighlight;
        }

        SecurityImageSelection {
            id: securityImageSelection;
            // Anchors
            anchors.top: securityImageTitle.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: 300;
        
            Connections {
                onSendSignalToWallet: {
                    sendSignalToWallet(msg);
                }
            }
        }

        // Navigation Bar
        Item {
            id: securityImageNavBar;
            // Size
            width: parent.width;
            height: 40;
            // Anchors:
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 30;

            // "Cancel" button
            HifiControlsUit.Button {
                color: hifi.buttons.noneBorderlessWhite;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: 150;
                text: "Cancel"
                onClicked: {
                    sendSignalToWallet({method: "walletSecurity_changeSecurityImageCancelled"});
                }
            }

            // "Submit" button
            HifiControlsUit.Button {
                id: securityImageSubmitButton;
                text: root.justSubmitted ? "Submitting..." : "Submit";
                enabled: securityImageSelection.currentIndex !== -1 && !root.justSubmitted;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: 150;
                onClicked: {
                    root.justSubmitted = true;
                    var securityImagePath = securityImageSelection.getImagePathFromImageID(securityImageSelection.getSelectedImageIndex())
                    Commerce.chooseSecurityImage(securityImagePath);
                }
            }
        }
    }
    //
    // SECURITY IMAGE SELECTION END
    //

    signal sendSignalToWallet(var msg);

    function initModel() {
        securityImageSelection.initModel();
    }
}
