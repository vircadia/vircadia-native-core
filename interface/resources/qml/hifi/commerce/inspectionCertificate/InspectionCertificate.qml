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

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string marketplaceUrl: "";
    property string entityId: "";
    property string certificateId: "";
    property string itemName: "--";
    property string itemOwner: "--";
    property string itemEdition: "--";
    property string dateOfPurchase: "--";
    property string itemCost: "--";
    property string certTitleTextColor: hifi.colors.darkGray;
    property string certTextColor: hifi.colors.white;
    property string infoTextColor: hifi.colors.blueAccent;
    // 0 means replace none
    // 4 means replace all but "Item Edition"
    // 5 means replace all 5 replaceable fields
    property int certInfoReplaceMode: 5;
    property bool isLightbox: false;
    property bool isMyCert: false;
    property bool useGoldCert: true;
    property bool certificateInfoPending: true;
    property int certificateStatus: 0;
    property bool certificateStatusPending: true;
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

                if (root.certInfoReplaceMode > 3) {
                    root.itemName = result.data.marketplace_item_name;
                    // "\u2022" is the Unicode character 'BULLET' - it's what's used in password fields on the web, etc
                    root.itemOwner = root.isMyCert ? Account.username :
                    "\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022";
                    root.dateOfPurchase = root.isMyCert ? getFormattedDate(result.data.transfer_created_at * 1000) : "Undisclosed";
                    root.itemCost = (root.isMyCert && result.data.cost !== undefined) ? result.data.cost : "Undisclosed";
                }
                if (root.certInfoReplaceMode > 4) {
                    root.itemEdition = result.data.edition_number + "/" + (result.data.limited_run === -1 ? "\u221e" : result.data.limited_run);
                }

                if (root.certificateStatus === 4) { // CERTIFICATE_STATUS_OWNER_VERIFICATION_FAILED
                    if (root.isMyCert) {
                        errorText.text = "This item is an uncertified copy of an item you purchased.";
                    } else {
                        errorText.text = "The person who placed this item doesn't own it.";
                    }
                }

                if (result.data.invalid_reason || result.data.transfer_status[0] === "failed") {
                    root.useGoldCert = false;
                    root.certTitleTextColor = hifi.colors.redHighlight;
                    root.certTextColor = hifi.colors.redHighlight;
                    root.infoTextColor = hifi.colors.redHighlight;
                    titleBarText.text = "Certificate\nNo Longer Valid";
                    popText.text = "";
                    showInMarketplaceButton.visible = false;
                    // "Edition" text previously set above in this function
                    // "Owner" text previously set above in this function
                    // "Purchase Date" text previously set above in this function
                    // "Purchase Price" text previously set above in this function
                    if (result.data.invalid_reason) {
                        errorText.text = result.data.invalid_reason;
                    }
                } else if (result.data.transfer_status[0] === "pending") {
                    root.useGoldCert = false;
                    root.certTitleTextColor = hifi.colors.redHighlight;
                    root.certTextColor = hifi.colors.redHighlight;
                    root.infoTextColor = hifi.colors.redHighlight;
                    titleBarText.text = "Certificate Pending";
                    popText.text = "";
                    showInMarketplaceButton.visible = true;
                    // "Edition" text previously set above in this function
                    // "Owner" text previously set above in this function
                    // "Purchase Date" text previously set above in this function
                    // "Purchase Price" text previously set above in this function
                    errorText.text = "The status of this item is still pending confirmation. If the purchase is not confirmed, " +
                    "this entity will be cleaned up by the domain.";
                }
            }
            root.certificateInfoPending = false;
        }

        onUpdateCertificateStatus: {
            updateCertificateStatus(certStatus);
        }
    }

    function updateCertificateStatus(status) {
        root.certificateStatus = status;
        if (root.certificateStatus === 1) { // CERTIFICATE_STATUS_VERIFICATION_SUCCESS
            root.useGoldCert = true;
            root.certTitleTextColor = hifi.colors.darkGray;
            root.certTextColor = hifi.colors.white;
            root.infoTextColor = hifi.colors.blueAccent;
            titleBarText.text = "Certificate";
            popText.text = "PROOF OF PROVENANCE";
            showInMarketplaceButton.visible = true;
            root.certInfoReplaceMode = 5;
            // "Item Name" text will be set in "onCertificateInfoResult()"
            // "Edition" text will be set in "onCertificateInfoResult()"
            // "Owner" text will be set in "onCertificateInfoResult()"
            // "Purchase Date" text will be set in "onCertificateInfoResult()"
            // "Purchase Price" text will be set in "onCertificateInfoResult()"
            errorText.text = "";
        } else if (root.certificateStatus === 2) { // CERTIFICATE_STATUS_VERIFICATION_TIMEOUT
            root.useGoldCert = false;
            root.certTitleTextColor = hifi.colors.redHighlight;
            root.certTextColor = hifi.colors.redHighlight;
            root.infoTextColor = hifi.colors.redHighlight;
            titleBarText.text = "Request Timed Out";
            popText.text = "";
            showInMarketplaceButton.visible = false;
            root.certInfoReplaceMode = 0;
            root.itemName = "";
            root.itemEdition = "";
            root.itemOwner = "";
            root.dateOfPurchase = "";
            root.itemCost = "";
            errorText.text = "Your request to inspect this item timed out. Please try again later.";
        } else if (root.certificateStatus === 3) { // CERTIFICATE_STATUS_STATIC_VERIFICATION_FAILED
            root.useGoldCert = false;
            root.certTitleTextColor = hifi.colors.redHighlight;
            root.certTextColor = hifi.colors.redHighlight;
            root.infoTextColor = hifi.colors.redHighlight;
            titleBarText.text = "Certificate\nNo Longer Valid";
            popText.text = "";
            showInMarketplaceButton.visible = true;
            root.certInfoReplaceMode = 5;
            // "Item Name" text will be set in "onCertificateInfoResult()"
            // "Edition" text will be set in "onCertificateInfoResult()"
            // "Owner" text will be set in "onCertificateInfoResult()"
            // "Purchase Date" text will be set in "onCertificateInfoResult()"
            // "Purchase Price" text will be set in "onCertificateInfoResult()"
            errorText.text = "The information associated with this item has been modified and it no longer matches the original certified item.";
        } else if (root.certificateStatus === 4) { // CERTIFICATE_STATUS_OWNER_VERIFICATION_FAILED
            root.useGoldCert = false;
            root.certTitleTextColor = hifi.colors.redHighlight;
            root.certTextColor = hifi.colors.redHighlight;
            root.infoTextColor = hifi.colors.redHighlight;
            titleBarText.text = "Invalid Certificate";
            popText.text = "";
            showInMarketplaceButton.visible = true;
            root.certInfoReplaceMode = 4;
            // "Item Name" text will be set in "onCertificateInfoResult()"
            root.itemEdition = "Uncertified Copy"
            // "Owner" text will be set in "onCertificateInfoResult()"
            // "Purchase Date" text will be set in "onCertificateInfoResult()"
            // "Purchase Price" text will be set in "onCertificateInfoResult()"
            // "Error Text" text will be set in "onCertificateInfoResult()"
        } else {
            console.log("Unknown certificate status received from ledger signal!");
        }
            
        root.certificateStatusPending = false;
        // We've gotten cert status - we are GO on getting the cert info
        Commerce.certificateInfo(root.certificateId);
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    Rectangle {
        id: loadingOverlay;
        z: 998;

        visible: root.certificateInfoPending || root.certificateStatusPending;
        anchors.fill: parent;
        color: Qt.rgba(0.0, 0.0, 0.0, 0.7);

        // This object is always used in a popup or full-screen Wallet section.
        // This MouseArea is used to prevent a user from being
        //     able to click on a button/mouseArea underneath the popup/section.
        MouseArea {
            anchors.fill: parent;
            propagateComposedEvents: false;
        }
                
        AnimatedImage {
            source: "../common/images/loader.gif"
            width: 96;
            height: width;
            anchors.verticalCenter: parent.verticalCenter;
            anchors.horizontalCenter: parent.horizontalCenter;
        }
    }

    Image {
        id: backgroundImage;
        anchors.fill: parent;
        source: root.useGoldCert ? "images/cert-bg-gold-split.png" : "images/nocert-bg-split.png";
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
        anchors.leftMargin: 36;
        anchors.right: parent.right;
        anchors.rightMargin: 8;
        height: paintedHeight;
        // Style
        color: root.certTitleTextColor;
        wrapMode: Text.WordWrap;
    }
    // Title text
    RalewayRegular {
        id: popText;
        // Text size
        size: 16;
        // Anchors
        anchors.top: titleBarText.bottom;
        anchors.topMargin: 4;
        anchors.left: titleBarText.left;
        anchors.right: titleBarText.right;
        height: text === "" ? 0 : paintedHeight;
        // Style
        color: root.certTitleTextColor;
    }
            
    // "Close" button
    HiFiGlyphs {
        z: 999; 
        id: closeGlyphButton;
        text: hifi.glyphs.close;
        color: hifi.colors.white;
        size: 26;
        anchors.top: parent.top;
        anchors.topMargin: 10;
        anchors.right: parent.right;
        anchors.rightMargin: 10;
        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true;
            onEntered: {
                parent.text = hifi.glyphs.closeInverted;
            }
            onExited: {
                parent.text = hifi.glyphs.close;
            }
            onClicked: {
                if (root.isLightbox) {
                    root.visible = false;
                } else {
                    sendToScript({method: 'inspectionCertificate_closeClicked', closeGoesToPurchases: root.closeGoesToPurchases});
                }
            }
        }
    }

    //
    // "CERTIFICATE" START
    //
    Item {
        id: certificateContainer;
        anchors.top: titleBarText.top;
        anchors.topMargin: 110;
        anchors.bottom: infoContainer.top;
        anchors.left: parent.left;
        anchors.leftMargin: titleBarText.anchors.leftMargin;
        anchors.right: parent.right;
        anchors.rightMargin: 24;

        RalewayRegular {
            id: itemNameHeader;
            text: "ITEM NAME";
            // Text size
            size: 16;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
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
            color: root.certTextColor;
            elide: Text.ElideRight;
            MouseArea {
                enabled: showInMarketplaceButton.visible;
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    sendToScript({method: 'inspectionCertificate_showInMarketplaceClicked', marketplaceUrl: root.marketplaceUrl});
                }
                onEntered: itemName.color = hifi.colors.blueHighlight;
                onExited: itemName.color = root.certTextColor;
            }
        }

        RalewayRegular {
            id: editionHeader;
            text: "EDITION";
            // Text size
            size: 16;
            // Anchors
            anchors.top: itemName.bottom;
            anchors.topMargin: 28;
            anchors.left: parent.left;
            anchors.right: parent.right;
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
            color: root.certTextColor;
        }

        // "Show In Marketplace" button
        HifiControlsUit.Button {
            id: showInMarketplaceButton;
            enabled: root.marketplaceUrl;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.light;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 48;
            anchors.right: parent.right;
            width: 200;
            height: 40;
            text: "View In Market"
            onClicked: {
                sendToScript({method: 'inspectionCertificate_showInMarketplaceClicked', marketplaceUrl: root.marketplaceUrl});
            }
        }
    }
    //
    // "CERTIFICATE" END
    //

    //
    // "INFO CONTAINER" START
    //
    Item {
        id: infoContainer;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.leftMargin: titleBarText.anchors.leftMargin;
        anchors.right: parent.right;
        anchors.rightMargin: 24;
        height: root.useGoldCert ? 220 : 372;

        RalewayRegular {
            id: errorText;
            visible: !root.useGoldCert;
            // Text size
            size: 20;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 36;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 116;
            // Style
            wrapMode: Text.WordWrap;
            color: hifi.colors.baseGray;
            verticalAlignment: Text.AlignTop;
        }

        RalewayRegular {
            id: ownedByHeader;
            text: "OWNER";
            // Text size
            size: 16;
            // Anchors
            anchors.top: errorText.visible ? errorText.bottom : parent.top;
            anchors.topMargin: 28;
            anchors.left: parent.left;
            anchors.right: parent.right;
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
            color: root.infoTextColor;
            elide: Text.ElideRight;
        }
        AnonymousProRegular {
            id: isMyCertText;
            visible: root.isMyCert && ownedBy.text !== "--" && ownedBy.text !== "";
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
            color: root.infoTextColor;
            elide: Text.ElideRight;
            verticalAlignment: Text.AlignVCenter;
        }

        RalewayRegular {
            id: dateOfPurchaseHeader;
            text: "PURCHASE DATE";
            // Text size
            size: 16;
            // Anchors
            anchors.top: ownedBy.bottom;
            anchors.topMargin: 28;
            anchors.left: parent.left;
            anchors.right: parent.horizontalCenter;
            anchors.rightMargin: 8;
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
            color: root.infoTextColor;
        }

        RalewayRegular {
            id: priceHeader;
            text: "PURCHASE PRICE";
            // Text size
            size: 16;
            // Anchors
            anchors.top: ownedBy.bottom;
            anchors.topMargin: 28;
            anchors.left: parent.horizontalCenter;
            anchors.right: parent.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.darkGray;
        }
        HiFiGlyphs {
            id: hfcGlyph;
            visible: priceText.text !== "Undisclosed" && priceText.text !== "";
            text: hifi.glyphs.hfc;
            // Size
            size: 24;
            // Anchors
            anchors.top: priceHeader.bottom;
            anchors.topMargin: 8;
            anchors.left: priceHeader.left;
            width: visible ? paintedWidth + 6 : 0;
            height: 40;
            // Style
            color: root.infoTextColor;
            verticalAlignment: Text.AlignTop;
            horizontalAlignment: Text.AlignLeft;
        }
        AnonymousProRegular {
            id: priceText;
            text: root.itemCost;
            // Text size
            size: 18;
            // Anchors
            anchors.top: priceHeader.bottom;
            anchors.topMargin: 8;
            anchors.left: hfcGlyph.right;
            anchors.right: priceHeader.right;
            height: paintedHeight;
            // Style
            color: root.infoTextColor;
        }
    }
    //
    // "INFO CONTAINER" END
    //

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
                resetCert(false);
                root.certificateId = message.certificateId;
                if (message.entityId === "") {
                    updateCertificateStatus(1); // CERTIFICATE_STATUS_VERIFICATION_SUCCESS
                } else {
                    root.entityId = message.entityId;
                    sendToScript({method: 'inspectionCertificate_requestOwnershipVerification', entity: root.entityId});
                }
            break;
            case 'inspectionCertificate_resetCert':
                resetCert(true);
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    function resetCert(alsoResetCertID) {
        if (alsoResetCertID) {
            root.entityId = "";
            root.certificateId = "";
        }
        root.certInfoReplaceMode = 5;
        root.certificateInfoPending = true;
        root.certificateStatusPending = true;
        root.useGoldCert = true;
        root.certTitleTextColor = hifi.colors.darkGray;
        root.certTextColor = hifi.colors.white;
        root.infoTextColor = hifi.colors.blueAccent;
        titleBarText.text = "Certificate";
        popText.text = "";
        root.itemName = "--";
        root.itemOwner = "--";
        root.itemEdition = "--";
        root.dateOfPurchase = "--";
        root.marketplaceUrl = "";
        root.itemCost = "--";
        root.isMyCert = false;
        errorText.text = "";
    }

    function getFormattedDate(timestamp) {
        if (timestamp === "--") {
            return "--";
        }
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
        return year + '-' + month + '-' + day + ' ' + drawnHour + ':' + min + amOrPm;
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
