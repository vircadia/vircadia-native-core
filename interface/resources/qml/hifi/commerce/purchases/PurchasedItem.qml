//
//  PurchasedItem.qml
//  qml/hifi/commerce/purchases
//
//  PurchasedItem
//
//  Created by Zach Fox on 2017-08-25
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls
import "../wallet" as HifiWallet

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    property string purchaseStatus;
    property bool purchaseStatusChanged;
    property bool canRezCertifiedItems: false;
    property string itemName;
    property string itemId;
    property string itemPreviewImageUrl;
    property string itemHref;
    property string certificateId;
    property int displayedItemCount;
    property int itemEdition;
    property int numberSold;
    property int limitedRun;
    property bool isWearable;

    property string originalStatusText;
    property string originalStatusColor;

    height: 110;
    width: parent.width;

    onPurchaseStatusChangedChanged: {
        if (root.purchaseStatusChanged === true && root.purchaseStatus === "confirmed") {
            root.originalStatusText = statusText.text;
            root.originalStatusColor = statusText.color;
            statusText.text = "CONFIRMED!";
            statusText.color = hifi.colors.blueAccent;
            confirmedTimer.start();
        }
    }

    Timer {
        id: confirmedTimer;
        interval: 3000;
        onTriggered: {
            statusText.text = root.originalStatusText;
            statusText.color = root.originalStatusColor;
            root.purchaseStatusChanged = false;
        }
    }

    Rectangle {
        id: mainContainer;
        // Style
        color: hifi.colors.white;
        // Size
        anchors.left: parent.left;
        anchors.leftMargin: 8;
        anchors.right: parent.right;
        anchors.rightMargin: 8;
        anchors.top: parent.top;
        height: root.height - 10;

        Image {
            id: itemPreviewImage;
            source: root.itemPreviewImageUrl;
            anchors.left: parent.left;
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            width: height;
            fillMode: Image.PreserveAspectCrop;

            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    sendToPurchases({method: 'purchases_itemInfoClicked', itemId: root.itemId});
                }
            }
        }

    
        RalewaySemiBold {
            id: itemName;
            anchors.top: itemPreviewImage.top;
            anchors.topMargin: 4;
            anchors.left: itemPreviewImage.right;
            anchors.leftMargin: 8;
            anchors.right: buttonContainer.left;
            anchors.rightMargin: 8;
            height: paintedHeight;
            // Text size
            size: 24;
            // Style
            color: hifi.colors.blueAccent;
            text: root.itemName;
            elide: Text.ElideRight;
            // Alignment
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter;

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    sendToPurchases({method: 'purchases_itemInfoClicked', itemId: root.itemId});
                }
                onEntered: {
                    itemName.color = hifi.colors.blueHighlight;
                }
                onExited: {
                    itemName.color = hifi.colors.blueAccent;
                }
            }
        }

        Item {
            id: certificateContainer;
            anchors.top: itemName.bottom;
            anchors.topMargin: 4;
            anchors.left: itemName.left;
            anchors.right: buttonContainer.left;
            anchors.rightMargin: 2;
            height: 24;
        
            HiFiGlyphs {
                id: certificateIcon;
                text: hifi.glyphs.scriptNew;
                // Size
                size: 30;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.bottom: parent.bottom;
                width: 32;
                // Style
                color: hifi.colors.lightGray;
            }

            RalewayRegular {
                id: viewCertificateText;
                text: "VIEW CERTIFICATE";
                size: 14;
                anchors.left: certificateIcon.right;
                anchors.leftMargin: 4;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                color: hifi.colors.lightGray;
            }

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    sendToPurchases({method: 'purchases_itemCertificateClicked', itemCertificateId: root.certificateId});
                }
                onEntered: {
                    certificateIcon.color = hifi.colors.black;
                    viewCertificateText.color = hifi.colors.black;
                }
                onExited: {
                    certificateIcon.color = hifi.colors.lightGray;
                    viewCertificateText.color = hifi.colors.lightGray;
                }
            }
        }

        Item {
            id: editionContainer;
            visible: root.displayedItemCount > 1 && !statusContainer.visible;
            anchors.left: itemName.left;
            anchors.top: certificateContainer.bottom;
            anchors.topMargin: 8;
            anchors.bottom: parent.bottom;
            anchors.right: buttonContainer.left;
            anchors.rightMargin: 2;

            FiraSansRegular {
                anchors.left: parent.left;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                text: "#" + root.itemEdition;
                size: 15;
                color: "#cc6a6a6a";
                verticalAlignment: Text.AlignTop;
            }
        }

        Item {
            id: statusContainer;
            visible: root.purchaseStatus === "pending" || root.purchaseStatus === "invalidated" || root.purchaseStatusChanged;
            anchors.left: itemName.left;
            anchors.top: certificateContainer.bottom;
            anchors.topMargin: 8;
            anchors.bottom: parent.bottom;
            anchors.right: buttonContainer.left;
            anchors.rightMargin: 2;

            RalewaySemiBold {
                id: statusText;
                anchors.left: parent.left;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                text: {
                        if (root.purchaseStatus === "pending") {
                            "PENDING..."
                        } else if (root.purchaseStatus === "invalidated") {
                            "INVALIDATED"
                        } else if (root.numberSold !== -1) {
                            ("Sales: " + root.numberSold + "/" + (root.limitedRun === -1 ? "\u221e" : root.limitedRun))
                        } else {
                            ""
                        }
                    }
                size: 18;
                color: {
                        if (root.purchaseStatus === "pending") {
                            hifi.colors.blueAccent
                        } else if (root.purchaseStatus === "invalidated") {
                            hifi.colors.redAccent
                        } else {
                            hifi.colors.baseGray
                        }
                    }
                verticalAlignment: Text.AlignTop;
            }
        
            HiFiGlyphs {
                id: statusIcon;
                text: {
                        if (root.purchaseStatus === "pending") {
                            hifi.glyphs.question
                        } else if (root.purchaseStatus === "invalidated") {
                            hifi.glyphs.question
                        } else {
                            ""
                        }
                    }
                // Size
                size: 36;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: -8;
                anchors.left: statusText.right;
                anchors.bottom: parent.bottom;
                // Style
                color: {
                        if (root.purchaseStatus === "pending") {
                            hifi.colors.blueAccent
                        } else if (root.purchaseStatus === "invalidated") {
                            hifi.colors.redAccent
                        } else {
                            hifi.colors.baseGray
                        }
                    }
                verticalAlignment: Text.AlignTop;
            }

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    if (root.purchaseStatus === "pending") {
                        sendToPurchases({method: 'showPendingLightbox'});
                    } else if (root.purchaseStatus === "invalidated") {
                        sendToPurchases({method: 'showInvalidatedLightbox'});
                    }
                }
                onEntered: {
                    if (root.purchaseStatus === "pending") {
                        statusText.color = hifi.colors.blueHighlight;
                        statusIcon.color = hifi.colors.blueHighlight;
                    } else if (root.purchaseStatus === "invalidated") {
                        statusText.color = hifi.colors.redAccent;
                        statusIcon.color = hifi.colors.redAccent;
                    }
                }
                onExited: {
                    if (root.purchaseStatus === "pending") {
                        statusText.color = hifi.colors.blueAccent;
                        statusIcon.color = hifi.colors.blueAccent;
                    } else if (root.purchaseStatus === "invalidated") {
                        statusText.color = hifi.colors.redHighlight;
                        statusIcon.color = hifi.colors.redHighlight;
                    }
                }
            }
        }

        Rectangle {
            id: rezzedNotifContainer;
            z: 998;
            visible: false;
            color: hifi.colors.blueHighlight;
            anchors.fill: buttonContainer;
            MouseArea {
                anchors.fill: parent;
                propagateComposedEvents: false;
                hoverEnabled: true;
            }

            RalewayBold {
                anchors.fill: parent;
                text: "REZZED";
                size: 18;
                color: hifi.colors.white;
                verticalAlignment: Text.AlignVCenter;
                horizontalAlignment: Text.AlignHCenter;
            }

                Timer {
                    id: rezzedNotifContainerTimer;
                    interval: 2000;
                    onTriggered: rezzedNotifContainer.visible = false
                }
        }

        Button {
            id: buttonContainer;
            property int color: hifi.buttons.red;
            property int colorScheme: hifi.colorSchemes.light;

            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            anchors.right: parent.right;
            width: height;
            enabled: (root.canRezCertifiedItems || root.isWearable) && root.purchaseStatus !== "invalidated";
            
            onClicked: {
                sendToPurchases({method: 'purchases_rezClicked', itemHref: root.itemHref, isWearable: root.isWearable});
                rezzedNotifContainer.visible = true;
                rezzedNotifContainerTimer.start();
                UserActivityLogger.commerceEntityRezzed(root.itemId, "purchases", root.isWearable ? "rez" : "wear");
            }

            style: ButtonStyle {

                background: Rectangle {
                    gradient: Gradient {
                        GradientStop {
                            position: 0.2
                            color: {
                                if (!control.enabled) {
                                    hifi.buttons.disabledColorStart[control.colorScheme]
                                } else if (control.pressed) {
                                    hifi.buttons.pressedColor[control.color]
                                } else if (control.hovered) {
                                    hifi.buttons.hoveredColor[control.color]
                                } else {
                                    hifi.buttons.colorStart[control.color]
                                }
                            }
                        }
                        GradientStop {
                            position: 1.0
                            color: {
                                if (!control.enabled) {
                                    hifi.buttons.disabledColorFinish[control.colorScheme]
                                } else if (control.pressed) {
                                    hifi.buttons.pressedColor[control.color]
                                } else if (control.hovered) {
                                    hifi.buttons.hoveredColor[control.color]
                                } else {
                                    hifi.buttons.colorFinish[control.color]
                                }
                            }
                        }
                    }
                }

                label: Item {
                    HiFiGlyphs {
                        id: lightningIcon;
                        text: hifi.glyphs.lightning;
                        // Size
                        size: 32;
                        // Anchors
                        anchors.top: parent.top;
                        anchors.topMargin: 12;
                        anchors.left: parent.left;
                        anchors.right: parent.right;
                        horizontalAlignment: Text.AlignHCenter;
                        // Style
                        color: enabled ? hifi.buttons.textColor[control.color]
                                       : hifi.buttons.disabledTextColor[control.colorScheme]
                    }
                    RalewayBold {
                        anchors.top: lightningIcon.bottom;
                        anchors.topMargin: -20;
                        anchors.right: parent.right;
                        anchors.left: parent.left;
                        anchors.bottom: parent.bottom;
                        font.capitalization: Font.AllUppercase
                        color: enabled ? hifi.buttons.textColor[control.color]
                                       : hifi.buttons.disabledTextColor[control.colorScheme]
                        size: 16;
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: root.isWearable ? "Wear It" : "Rez It"
                    }
                }
            }
        }
    }

    DropShadow {
        anchors.fill: mainContainer;
        horizontalOffset: 3;
        verticalOffset: 3;
        radius: 8.0;
        samples: 17;
        color: "#80000000";
        source: mainContainer;
    }

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToPurchases(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
