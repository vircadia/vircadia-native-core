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
import TabletScriptingInterface 1.0

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    property string purchaseStatus;
    property bool purchaseStatusChanged;
    property string itemName;
    property string itemId;
    property string itemPreviewImageUrl;
    property string itemHref;
    property string certificateId;
    property int displayedItemCount;
    property int itemEdition;
    property int numberSold;
    property int limitedRun;
    property string itemType;
    property var itemTypesArray: ["entity", "wearable", "contentSet", "app", "avatar"];
    property var buttonTextNormal: ["REZ", "WEAR", "REPLACE", "INSTALL", "WEAR"];
    property var buttonTextClicked: ["REZZED", "WORN", "REPLACED", "INSTALLED", "WORN"]
    property var buttonGlyph: [hifi.glyphs.wand, hifi.glyphs.hat, hifi.glyphs.globe, hifi.glyphs.install, hifi.glyphs.avatar];
    property bool showConfirmation: false;
    property bool hasPermissionToRezThis;
    property bool permissionExplanationCardVisible;
    property bool isInstalled;

    property string originalStatusText;
    property string originalStatusColor;

    height: 110;
    width: parent.width;

    Connections {
        target: Commerce;
        
        onContentSetChanged: {
            if (contentSetHref === root.itemHref) {
                showConfirmation = true;
            }
        }

        onAppInstalled: {
            if (appHref === root.itemHref) {
                root.isInstalled = true;
            }
        }

        onAppUninstalled: {
            if (appHref === root.itemHref) {
                root.isInstalled = false;
            }
        }
    }

    Connections {
        target: MyAvatar;

        onSkeletonModelURLChanged: {
            if (skeletonModelURL === root.itemHref) {
                showConfirmation = true;
            }
        }
    }

    onItemTypeChanged: {
        if ((itemType === "entity" && (!Entities.canRezCertified() && !Entities.canRezTmpCertified())) ||
            (itemType === "contentSet" && !Entities.canReplaceContent())) {
            root.hasPermissionToRezThis = false;
        } else {
            root.hasPermissionToRezThis = true;
        }
    }

    onPurchaseStatusChangedChanged: {
        if (root.purchaseStatusChanged === true && root.purchaseStatus === "confirmed") {
            root.originalStatusText = statusText.text;
            root.originalStatusColor = statusText.color;
            statusText.text = "CONFIRMED!";
            statusText.color = hifi.colors.blueAccent;
            confirmedTimer.start();
        }
    }

    onShowConfirmationChanged: {
        if (root.showConfirmation) {
            rezzedNotifContainer.visible = true;
            rezzedNotifContainerTimer.start();
            UserActivityLogger.commerceEntityRezzed(root.itemId, "purchases", root.itemType);
            root.showConfirmation = false;
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
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
        anchors.verticalCenter: parent.verticalCenter;
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

        TextMetrics {
            id:     itemNameTextMetrics;
            font:   itemName.font;
            text:   itemName.text;
        }
        RalewaySemiBold {
            id: itemName;
            anchors.top: itemPreviewImage.top;
            anchors.topMargin: 4;
            anchors.left: itemPreviewImage.right;
            anchors.leftMargin: 8;
            width: !noPermissionGlyph.visible ? (buttonContainer.x - itemPreviewImage.x - itemPreviewImage.width - anchors.leftMargin) :
                Math.min(itemNameTextMetrics.tightBoundingRect.width + 2,
                buttonContainer.x - itemPreviewImage.x - itemPreviewImage.width - anchors.leftMargin - noPermissionGlyph.width + 2);
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
        HiFiGlyphs {
            id: noPermissionGlyph;
            visible: !root.hasPermissionToRezThis;
            anchors.verticalCenter: itemName.verticalCenter;
            anchors.left: itemName.right;
            anchors.leftMargin: itemName.truncated ? -10 : -2;
            text: hifi.glyphs.info;
            // Size
            size: 40;
            width: 32;
            // Style
            color: hifi.colors.redAccent;
            
            MouseArea {
                anchors.fill: parent;
                hoverEnabled: true;

                onEntered: {
                    noPermissionGlyph.color = hifi.colors.redHighlight;
                }
                onExited: {
                    noPermissionGlyph.color = hifi.colors.redAccent;
                }
                onClicked: {
                    root.sendToPurchases({ method: 'openPermissionExplanationCard' });
                }
            }
        }
        Rectangle {
            id: permissionExplanationCard;
            z: 995;
            visible: root.permissionExplanationCardVisible;
            anchors.fill: parent;
            color: hifi.colors.white;

            RalewayRegular {
                id: permissionExplanationText;
                text: {
                    if (root.itemType === "contentSet") {
                        "You do not have 'Replace Content' permissions in this domain. <a href='#replaceContentPermission'>Learn more</a>";
                    } else if (root.itemType === "entity") {
                        "You do not have 'Rez Certified' permissions in this domain. <a href='#rezCertifiedPermission'>Learn more</a>";
                    } else {
                        ""
                    }
                }
                size: 16;
                anchors.left: parent.left;
                anchors.leftMargin: 30;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: permissionExplanationGlyph.left;
                color: hifi.colors.baseGray;
                wrapMode: Text.WordWrap;
                verticalAlignment: Text.AlignVCenter;

                onLinkActivated: {
                    sendToPurchases({method: 'showPermissionsExplanation', itemType: root.itemType});
                }
            }
            // "Close" button
            HiFiGlyphs {
                id: permissionExplanationGlyph;
                text: hifi.glyphs.close;
                color: hifi.colors.baseGray;
                size: 26;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                width: 77;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
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
                        root.sendToPurchases({ method: 'openPermissionExplanationCard', closeAll: true });
                    }
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
                color: hifi.colors.black;
            }

            RalewayRegular {
                id: viewCertificateText;
                text: "VIEW CERTIFICATE";
                size: 13;
                anchors.left: certificateIcon.right;
                anchors.leftMargin: 4;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                color: hifi.colors.black;
            }

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    sendToPurchases({method: 'purchases_itemCertificateClicked', itemCertificateId: root.certificateId});
                }
                onEntered: {
                    certificateIcon.color = hifi.colors.lightGray;
                    viewCertificateText.color = hifi.colors.lightGray;
                }
                onExited: {
                    certificateIcon.color = hifi.colors.black;
                    viewCertificateText.color = hifi.colors.black;
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

            RalewayRegular {
                anchors.left: parent.left;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                text: "#" + root.itemEdition;
                size: 13;
                color: hifi.colors.black;
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
            color: "#1FC6A6";
            anchors.fill: buttonContainer;
            MouseArea {
                anchors.fill: parent;
                propagateComposedEvents: false;
                hoverEnabled: true;
            }

            RalewayBold {
                anchors.fill: parent;
                text: (root.buttonTextClicked)[itemTypesArray.indexOf(root.itemType)];
                size: 15;
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

        Rectangle {
            id: appButtonContainer;
            color: hifi.colors.white;
            z: 994;
            visible: root.isInstalled;
            anchors.fill: buttonContainer;

            HifiControlsUit.Button {
                id: openAppButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.light;
                anchors.top: parent.top;
                anchors.right: parent.right;
                anchors.left: parent.left;
                width: 92;
                height: 44;
                text: "OPEN"
                onClicked: {
                    Commerce.openApp(root.itemHref);
                }
            }

            HifiControlsUit.Button {
                id: uninstallAppButton;
                color: hifi.buttons.noneBorderless;
                colorScheme: hifi.colorSchemes.light;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                anchors.left: parent.left;
                height: 44;
                text: "UNINSTALL"
                onClicked: {
                    Commerce.uninstallApp(root.itemHref);
                }
            }
        }

        Button {
            id: buttonContainer;
            property int color: hifi.buttons.blue;
            property int colorScheme: hifi.colorSchemes.light;

            anchors.top: parent.top;
            anchors.topMargin: 4;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 4;
            anchors.right: parent.right;
            anchors.rightMargin: 4;
            width: height;
            enabled: root.hasPermissionToRezThis &&
                root.purchaseStatus !== "invalidated" &&
                MyAvatar.skeletonModelURL !== root.itemHref;

            onHoveredChanged: {
                if (hovered) {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
            }
    
            onFocusChanged: {
                if (focus) {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
            }
            
            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                if (root.itemType === "contentSet") {
                    sendToPurchases({method: 'showReplaceContentLightbox', itemHref: root.itemHref});
                } else if (root.itemType === "avatar") {
                    sendToPurchases({method: 'showChangeAvatarLightbox', itemName: root.itemName, itemHref: root.itemHref});
                } else if (root.itemType === "app") {
                    // "Run" and "Uninstall" buttons are separate.
                    Commerce.installApp(root.itemHref);
                } else {
                    sendToPurchases({method: 'purchases_rezClicked', itemHref: root.itemHref, itemType: root.itemType});
                    root.showConfirmation = true;
                }
            }

            style: ButtonStyle {
                background: Rectangle {
                    radius: 4;
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
                        id: rezIcon;
                        text: (root.buttonGlyph)[itemTypesArray.indexOf(root.itemType)];
                        // Size
                        size: 60;
                        // Anchors
                        anchors.top: parent.top;
                        anchors.topMargin: 0;
                        anchors.left: parent.left;
                        anchors.right: parent.right;
                        horizontalAlignment: Text.AlignHCenter;
                        // Style
                        color: enabled ? hifi.buttons.textColor[control.color]
                                       : hifi.buttons.disabledTextColor[control.colorScheme]
                    }
                    RalewayBold {
                        id: rezIconLabel;
                        anchors.top: rezIcon.bottom;
                        anchors.topMargin: -4;
                        anchors.right: parent.right;
                        anchors.left: parent.left;
                        anchors.bottom: parent.bottom;
                        font.capitalization: Font.AllUppercase
                        color: enabled ? hifi.buttons.textColor[control.color]
                                       : hifi.buttons.disabledTextColor[control.colorScheme]
                        size: 15;
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: MyAvatar.skeletonModelURL === root.itemHref ? "CURRENT" : (root.buttonTextNormal)[itemTypesArray.indexOf(root.itemType)];
                    }
                }
            }
        }
    }

    DropShadow {
        anchors.fill: mainContainer;
        horizontalOffset: 0;
        verticalOffset: 4;
        radius: 4.0;
        samples: 9
        color: Qt.rgba(0, 0, 0, 0.25);
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
