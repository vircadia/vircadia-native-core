//
//  SecurityImageSelectionLightbox.qml
//  qml/hifi/commerce/wallet
//
//  SecurityImageSelectionLightbox
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

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property bool justSubmitted: false;
    // Style
    color: hifi.colors.baseGray;

    Hifi.QmlCommerce {
        id: commerce;

        onSecurityImageResult: {
            if (exists) { // Success submitting new security image
                if (root.justSubmitted) {
                    root.resetSubmitButton();
                    root.visible = false;
                    root.justSubmitted = false;
                }
            } else if (root.justSubmitted) {
                // Error submitting new security image.
                root.resetSubmitButton();
                root.justSubmitted = false;
            }
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
                text: "CHANGE SECURITY IMAGE";
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
            id: securityImageNavBar;
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
                    root.visible = false;
                }
            }

            // "Submit" button
            HifiControlsUit.Button {
                id: securityImageSubmitButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: 100;
                text: "Submit";
                onClicked: {
                    root.justSubmitted = true;
                    securityImageSubmitButton.text = "Submitting...";
                    securityImageSubmitButton.enabled = false;
                    var securityImagePath = securityImageSelection.getImagePathFromImageID(securityImageSelection.getSelectedImageIndex())
                    commerce.chooseSecurityImage(securityImagePath);
                }
            }
        }
    }
    //
    // SECURITY IMAGE SELECTION END
    //

    signal sendSignalToWallet(var msg);

    function resetSubmitButton() {
        securityImageSubmitButton.enabled = true;
        securityImageSubmitButton.text = "Submit";
    }
}
