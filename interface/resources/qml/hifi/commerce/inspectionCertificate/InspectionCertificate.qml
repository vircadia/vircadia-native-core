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
    property string marketplaceUrl;
    property string certificateId;
    property string itemName: "--";
    property string itemOwner: "--";
    property string itemEdition: "--";
    property string dateOfPurchase: "--";
    property bool isLightbox: false;
    property bool isMyCert: false;
    property bool isCertificateInvalid: false;
    // Style
    color: hifi.colors.faintGray;
    Connections {
        target: Commerce;

        onCertificateInfoResult: {
            if (result.status !== 'success') {
                console.log("Failed to get certificate info", result.message);
            } else {
                root.marketplaceUrl = result.data.marketplace_item_url;
                root.isMyCert = result.isMyCert ? result.isMyCert : false;
                root.itemOwner = root.isCertificateInvalid ? "--" : (root.isMyCert ? Account.username :
                "\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022");
                root.itemEdition = root.isCertificateInvalid ? "Uncertified Copy" :
                (result.data.edition_number + "/" + (result.data.limited_run === -1 ? "\u221e" : result.data.limited_run));
                root.dateOfPurchase = root.isCertificateInvalid ? "" : getFormattedDate(result.data.transfer_created_at * 1000);
                root.itemName = result.data.marketplace_item_name;

                if (result.data.invalid_reason || result.data.transfer_status[0] === "failed") {
                    titleBarText.text = "Invalid Certificate";
                    titleBarText.color = hifi.colors.redHighlight;
                    popText.text = "";
                    if (result.data.invalid_reason) {
                        errorText.text = result.data.invalid_reason;
                    }
                } else if (result.data.transfer_status[0] === "pending") {
                    titleBarText.text = "Certificate Pending";
                    errorText.text = "The status of this item is still pending confirmation. If the purchase is not confirmed, " +
                    "this entity will be cleaned up by the domain.";
                    errorText.color = hifi.colors.baseGray;
                }
            }
        }

        onUpdateCertificateStatus: {
            if (certStatus === 1) { // CERTIFICATE_STATUS_VERIFICATION_SUCCESS
                // NOP
            } else if (certStatus === 2) { // CERTIFICATE_STATUS_VERIFICATION_TIMEOUT
                root.isCertificateInvalid = true;
                errorText.text = "Verification of this certificate timed out.";
                errorText.color = hifi.colors.redHighlight;
            } else if (certStatus === 3) { // CERTIFICATE_STATUS_STATIC_VERIFICATION_FAILED
                root.isCertificateInvalid = true;
                titleBarText.text = "Invalid Certificate";
                titleBarText.color = hifi.colors.redHighlight;

                popText.text = "";
                root.itemOwner = "";
                dateOfPurchaseHeader.text = "";
                root.dateOfPurchase = "";
                root.itemEdition = "Uncertified Copy";

                errorText.text = "The information associated with this item has been modified and it no longer matches the original certified item.";
                errorText.color = hifi.colors.baseGray;
            } else if (certStatus === 4) { // CERTIFICATE_STATUS_OWNER_VERIFICATION_FAILED
                root.isCertificateInvalid = true;
                titleBarText.text = "Invalid Certificate";
                titleBarText.color = hifi.colors.redHighlight;

                popText.text = "";
                root.itemOwner = "";
                dateOfPurchaseHeader.text = "";
                root.dateOfPurchase = "";
                root.itemEdition = "Uncertified Copy";

                errorText.text = "The avatar who rezzed this item doesn't own it.";
                errorText.color = hifi.colors.baseGray;
            } else {
                console.log("Unknown certificate status received from ledger signal!");
            }
        }
    }

    onCertificateIdChanged: {
        if (certificateId !== "") {
            Commerce.certificateInfo(certificateId);
        }
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
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
        text: "Proof of Provenance";
        // Text size
        size: 16;
        // Anchors
        anchors.top: titleBarText.bottom;
        anchors.topMargin: 4;
        anchors.left: titleBarText.left;
        anchors.right: titleBarText.right;
        height: paintedHeight;
        // Style
        color: hifi.colors.darkGray;
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
            color: hifi.colors.darkGray;
        }
        RalewaySemiBold {
            id: itemName;
            text: root.itemName;
            // Text size
            size: 28;
            // Anchors
            anchors.top: itemNameHeader.bottom;
            anchors.topMargin: 8;
            anchors.left: itemNameHeader.left;
            anchors.right: itemNameHeader.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
            elide: Text.ElideRight;
            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    sendToScript({method: 'inspectionCertificate_showInMarketplaceClicked', marketplaceUrl: root.marketplaceUrl});
                }
                onEntered: itemName.color = hifi.colors.blueHighlight;
                onExited: itemName.color = hifi.colors.white;
            }
        }

        RalewayRegular {
            id: ownedByHeader;
            text: "OWNER";
            // Text size
            size: 16;
            // Anchors
            anchors.top: itemName.bottom;
            anchors.topMargin: 28;
            anchors.left: parent.left;
            anchors.leftMargin: 45;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.darkGray;
        }
        RalewayRegular {
            id: ownedBy;
            text: root.itemOwner;
            // Text size
            size: 22;
            // Anchors
            anchors.top: ownedByHeader.bottom;
            anchors.topMargin: 8;
            anchors.left: ownedByHeader.left;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
            elide: Text.ElideRight;
        }
        AnonymousProRegular {
            id: isMyCertText;
            visible: root.isMyCert && !root.isCertificateInvalid;
            text: "(Private)";
            size: 18;
            // Anchors
            anchors.top: ownedBy.top;
            anchors.topMargin: 4;
            anchors.bottom: ownedBy.bottom;
            anchors.left: ownedBy.right;
            anchors.leftMargin: 6;
            anchors.right: ownedByHeader.right;
            // Style
            color: hifi.colors.white;
            elide: Text.ElideRight;
            verticalAlignment: Text.AlignVCenter;
        }

        RalewayRegular {
            id: editionHeader;
            text: "EDITION";
            // Text size
            size: 16;
            // Anchors
            anchors.top: ownedBy.bottom;
            anchors.topMargin: 28;
            anchors.left: parent.left;
            anchors.leftMargin: 45;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.darkGray;
        }
        AnonymousProRegular {
            id: edition;
            text: root.itemEdition;
            // Text size
            size: 18;
            // Anchors
            anchors.top: editionHeader.bottom;
            anchors.topMargin: 8;
            anchors.left: editionHeader.left;
            anchors.right: editionHeader.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
        }

        RalewayRegular {
            id: dateOfPurchaseHeader;
            text: "DATE OF PURCHASE";
            // Text size
            size: 16;
            // Anchors
            anchors.top: edition.bottom;
            anchors.topMargin: 28;
            anchors.left: parent.left;
            anchors.leftMargin: 45;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            // Style
            color: hifi.colors.darkGray;
        }
        AnonymousProRegular {
            id: dateOfPurchase;
            text: root.dateOfPurchase;
            // Text size
            size: 18;
            // Anchors
            anchors.top: dateOfPurchaseHeader.bottom;
            anchors.topMargin: 8;
            anchors.left: dateOfPurchaseHeader.left;
            anchors.right: dateOfPurchaseHeader.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
        }

        RalewayRegular {
            id: errorText;
            // Text size
            size: 20;
            // Anchors
            anchors.top: dateOfPurchase.bottom;
            anchors.topMargin: 36;
            anchors.left: dateOfPurchase.left;
            anchors.right: dateOfPurchase.right;
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
        anchors.bottomMargin: 30;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: 50;
        
        // "Cancel" button
        HifiControlsUit.Button {
            color: hifi.buttons.noneBorderlessWhite;
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
            enabled: root.marketplaceUrl;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.light;
            anchors.top: parent.top;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            width: parent.width/2 - 50;
            height: 50;
            text: "View In Market"
            onClicked: {
                sendToScript({method: 'inspectionCertificate_showInMarketplaceClicked', marketplaceUrl: root.marketplaceUrl});
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
            case 'inspectionCertificate_setCertificateId':
                root.certificateId = message.certificateId;
            break;
            case 'inspectionCertificate_resetCert':
                titleBarText.text = "Certificate";
                popText.text = "PROOF OF PURCHASE";
                root.certificateId = "";
                root.itemName = "--";
                root.itemOwner = "--";
                root.itemEdition = "--";
                root.dateOfPurchase = "--";
                root.marketplaceUrl = "";
                root.isMyCert = false;
                errorText.text = "";
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    function getFormattedDate(timestamp) {
        function addLeadingZero(n) {
            return n < 10 ? '0' + n : '' + n;
        }

        var a = new Date(timestamp);
        var year = a.getFullYear();
        var month = addLeadingZero(a.getMonth() + 1);
        var day = addLeadingZero(a.getDate());
        var hour = a.getHours();
        var drawnHour = hour;
        if (hour === 0) {
            drawnHour = 12;
        } else if (hour > 12) {
            drawnHour -= 12;
        }
        drawnHour = addLeadingZero(drawnHour);
        
        var amOrPm = "AM";
        if (hour >= 12) {
            amOrPm = "PM";
        }

        var min = addLeadingZero(a.getMinutes());
        var sec = addLeadingZero(a.getSeconds());
        return year + '-' + month + '-' + day + '<br>' + drawnHour + ':' + min + amOrPm;
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
