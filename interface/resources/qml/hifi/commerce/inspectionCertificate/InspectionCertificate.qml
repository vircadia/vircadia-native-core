//
//  InspectionCertificate.qml
//  qml/hifi/commerce/inspectionCertificate
//
//  InspectionCertificate
//
//  Created by Zach Fox on 2017-09-14
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
import "../wallet" as HifiWallet

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string marketplaceId: "";
    property string itemName: "--";
    property string itemOwner: "--";
    property string itemEdition: "--";
    property string dateOfPurchase: "";
    property bool isLightbox: false;
    // Style
    color: hifi.colors.faintGray;
    Hifi.QmlCommerce {
        id: commerce;
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
    }

    Image {
        anchors.fill: parent;
        source: "images/cert-bg.jpg";
    }

    // Title text
    RalewayLight {
        id: titleBarText;
        text: "Certificate";
        // Text size
        size: 40;
        // Anchors
        anchors.top: parent.top;
        anchors.topMargin: 40;
        anchors.left: parent.left;
        anchors.leftMargin: 45;
        anchors.right: parent.right;
        height: paintedHeight;
        // Style
        color: hifi.colors.darkGray;
    }
    // Title text
    RalewayRegular {
        id: popText;
        text: "PROOF OF PURCHASE";
        // Text size
        size: 16;
        // Anchors
        anchors.top: titleBarText.bottom;
        anchors.topMargin: 4;
        anchors.left: titleBarText.left;
        anchors.right: titleBarText.right;
        height: paintedHeight;
        // Style
        color: hifi.colors.baseGray;
    }

    //
    // "CERTIFICATE" START
    //
    Item {
        id: certificateContainer;
        anchors.top: popText.bottom;
        anchors.topMargin: 30;
        anchors.bottom: buttonsContainer.top;
        anchors.left: parent.left;
        anchors.right: parent.right;

        RalewayRegular {
            id: itemNameHeader;
            text: "ITEM NAME";
            // Text size
            size: 16;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 45;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.baseGray;
        }
        RalewaySemiBold {
            id: itemName;
            text: root.itemName;
            // Text size
            size: 28;
            // Anchors
            anchors.top: itemNameHeader.bottom;
            anchors.topMargin: 4;
            anchors.left: itemNameHeader.left;
            anchors.right: itemNameHeader.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.blueAccent;
            elide: Text.ElideRight;
            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    sendToScript({method: 'inspectionCertificate_showInMarketplaceClicked', itemId: root.marketplaceId});
                }
                onEntered: itemName.color = hifi.colors.blueHighlight;
                onExited: itemName.color = hifi.colors.blueAccent;
            }
        }

        RalewayRegular {
            id: ownedByHeader;
            text: "OWNER";
            // Text size
            size: 16;
            // Anchors
            anchors.top: itemName.bottom;
            anchors.topMargin: 20;
            anchors.left: parent.left;
            anchors.leftMargin: 45;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.baseGray;
        }
        RalewayRegular {
            id: ownedBy;
            text: root.itemOwner;
            // Text size
            size: 22;
            // Anchors
            anchors.top: ownedByHeader.bottom;
            anchors.topMargin: 4;
            anchors.left: ownedByHeader.left;
            anchors.right: ownedByHeader.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.darkGray;
            elide: Text.ElideRight;
        }

        RalewayRegular {
            id: editionHeader;
            text: "EDITION";
            // Text size
            size: 16;
            // Anchors
            anchors.top: ownedBy.bottom;
            anchors.topMargin: 20;
            anchors.left: parent.left;
            anchors.leftMargin: 45;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.baseGray;
        }
        AnonymousProRegular {
            id: edition;
            text: root.itemEdition;
            // Text size
            size: 22;
            // Anchors
            anchors.top: editionHeader.bottom;
            anchors.topMargin: 4;
            anchors.left: editionHeader.left;
            anchors.right: editionHeader.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.darkGray;
        }

        RalewayRegular {
            id: dateOfPurchaseHeader;
            text: "DATE OF PURCHASE";
            visible: root.dateOfPurchase !== "";
            // Text size
            size: 16;
            // Anchors
            anchors.top: ownedBy.bottom;
            anchors.topMargin: 20;
            anchors.left: parent.left;
            anchors.leftMargin: 45;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.baseGray;
        }
        AnonymousProRegular {
            id: dateOfPurchase;
            text: root.dateOfPurchase;
            visible: root.dateOfPurchase !== "";
            // Text size
            size: 22;
            // Anchors
            anchors.top: editionHeader.bottom;
            anchors.topMargin: 4;
            anchors.left: editionHeader.left;
            anchors.right: editionHeader.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.darkGray;
        }

        RalewayRegular {
            id: errorText;
            text: "Here we will display some text if there's an <b>error</b> with the certificate " +
            "(DMCA takedown, invalid cert, location of item updated)";
            // Text size
            size: 20;
            // Anchors
            anchors.top: root.dateOfPurchase !== "" ? dateOfPurchase.bottom : edition.bottom;
            anchors.topMargin: 40;
            anchors.left: root.dateOfPurchase !== "" ? dateOfPurchase.left : edition.left;
            anchors.right: root.dateOfPurchase !== "" ? dateOfPurchase.right : edition.right;
            anchors.bottom: parent.bottom;
            // Style
            wrapMode: Text.WordWrap;
            color: hifi.colors.redHighlight;
            verticalAlignment: Text.AlignTop;
        }
    }
    //
    // "CERTIFICATE" END
    //

    Item {
        id: buttonsContainer;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 50;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: 50;
        
        // "Cancel" button
        HifiControlsUit.Button {
            color: hifi.buttons.noneBorderless;
            colorScheme: hifi.colorSchemes.light;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 30;
            width: parent.width/2 - 50;
            height: 50;
            text: "close";
            onClicked: {
                if (root.isLightbox) {
                    root.visible = false;
                } else {
                    sendToScript({method: 'inspectionCertificate_closeClicked', closeGoesToPurchases: root.closeGoesToPurchases});
                }
            }
        }   

        // "Show In Marketplace" button
        HifiControlsUit.Button {
            id: showInMarketplaceButton;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.light;
            anchors.top: parent.top;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            width: parent.width/2 - 50;
            height: 50;
            text: "View In Market"
            onClicked: {
                sendToScript({method: 'inspectionCertificate_showInMarketplaceClicked', itemId: root.marketplaceId});
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
    // message: The message sent from the JavaScript, in this case the Marketplaces JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from a script.
    //
    function fromScript(message) {
        switch (message.method) {
            case 'inspectionCertificate_setMarketplaceId':
                root.marketplaceId = message.marketplaceId;
            break;
            case 'inspectionCertificate_setItemInfo':
                root.itemName = message.itemName;
                root.itemOwner = message.itemOwner;
                root.itemEdition = message.itemEdition;
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);
    //
    // FUNCTION DEFINITIONS END
    //
}
