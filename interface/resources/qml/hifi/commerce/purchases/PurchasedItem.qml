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
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls
import "../wallet" as HifiWallet
import TabletScriptingInterface 1.0

Item {
    HifiConstants { id: hifi; }

    id: root;
    property string purchaseStatus;
    property string itemName;
    property string itemId;
    property string updateItemId;
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
    property bool cardBackVisible;
    property bool isInstalled;
    property string wornEntityID;
    property string updatedItemId;
    property string upgradeTitle;
    property bool updateAvailable: root.updateItemId !== "";
    property bool valid;
    property bool standaloneOptimized;
    property bool standaloneIncompatible;

    property string originalStatusText;
    property string originalStatusColor;

    height: 102;
    width: parent.width;

    Connections {
        target: Commerce;

        onContentSetChanged: {
            if (contentSetHref === root.itemHref) {
                showConfirmation = true;
            }
        }

        onAppInstalled: {
            if (appID === root.itemId) {
                root.isInstalled = true;
            }
        }

        onAppUninstalled: {
            if (appID === root.itemId) {
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

    onShowConfirmationChanged: {
        if (root.showConfirmation) {
            rezzedNotifContainer.visible = true;
            rezzedNotifContainerTimer.start();
            UserActivityLogger.commerceEntityRezzed(root.itemId, "purchases", root.itemType);
            root.showConfirmation = false;
        }
    }

    Rectangle {
        id: background;
        z: 10;
        color: Qt.rgba(0, 0, 0, 0.25);
        anchors.fill: parent;
    }

    Flipable {
        id: flipable;
        z: 50;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;
        height: root.height - 2;

        front: mainContainer;
        back: Rectangle {
            anchors.fill: parent;
            color: hifi.colors.white;

            Item {
                id: closeContextMenuContainer;
                anchors.right: parent.right;
                anchors.rightMargin: 8;
                anchors.top: parent.top;
                anchors.topMargin: 8;
                width: 30;
                height: width;

                HiFiGlyphs {
                    id: closeContextMenuGlyph;
                    text: hifi.glyphs.close;
                    anchors.fill: parent;
                    size: 26;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    color: hifi.colors.black;
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: enabled;
                    onClicked: {
                        root.sendToPurchases({ method: 'flipCard', closeAll: true });
                    }
                    onEntered: {
                        closeContextMenuGlyph.text = hifi.glyphs.closeInverted;
                    }
                    onExited: {
                        closeContextMenuGlyph.text = hifi.glyphs.close;
                    }
                }
            }

            Rectangle {
                id: contextCard;
                anchors.left: parent.left;
                anchors.leftMargin: 30;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: closeContextMenuContainer.left;
                anchors.rightMargin: 8;
                color: hifi.colors.white;

                Component {
                    id: contextCardButton;

                    Item {
                        property alias buttonGlyphText: buttonGlyph.text;
                        property alias itemButtonText: buttonText.text;
                        property alias glyphSize: buttonGlyph.size;
                        property string buttonColor: hifi.colors.black;
                        property string buttonColor_hover: hifi.colors.blueHighlight;
                        property alias enabled: buttonMouseArea.enabled;
                        property var buttonClicked;

                        HiFiGlyphs {
                            id: buttonGlyph;
                            anchors.top: parent.top;
                            anchors.topMargin: 4;
                            anchors.horizontalCenter: parent.horizontalCenter;
                            anchors.bottom: buttonText.visible ? parent.verticalCenter : parent.bottom;
                            anchors.bottomMargin: buttonText.visible ? 0 : 4;
                            width: parent.width;
                            size: 40;
                            horizontalAlignment: Text.AlignHCenter;
                            verticalAlignment: Text.AlignVCenter;
                            color: buttonMouseArea.enabled ? buttonColor : hifi.colors.lightGrayText;
                        }

                        RalewayRegular {
                            id: buttonText;
                            visible: text !== "";
                            anchors.top: parent.verticalCenter;
                            anchors.topMargin: 4;
                            anchors.bottom: parent.bottom;
                            anchors.bottomMargin: 12;
                            anchors.horizontalCenter: parent.horizontalCenter;
                            width: parent.width;
                            color: buttonMouseArea.enabled ? buttonColor : hifi.colors.lightGrayText;
                            size: 16;
                            wrapMode: Text.Wrap;
                            horizontalAlignment: Text.AlignHCenter;
                            verticalAlignment: Text.AlignVCenter;
                        }

                        MouseArea {
                            id: buttonMouseArea;
                            anchors.fill: parent;
                            hoverEnabled: enabled;
                            onClicked: {
                                parent.buttonClicked();
                            }
                            onEntered: {
                                buttonGlyph.color = buttonColor_hover;
                                buttonText.color = buttonColor_hover;
                            }
                            onExited: {
                                buttonGlyph.color = buttonColor;
                                buttonText.color = buttonColor;
                            }
                        }
                    }
                }

                Loader {
                    id: giftButton;
                    visible: root.itemEdition > 0;
                    sourceComponent: contextCardButton;
                    anchors.right: parent.right;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    width: 62;

                    onLoaded: {
                        item.enabled = root.valid;
                        item.buttonGlyphText = hifi.glyphs.gift;
                        item.itemButtonText = "Gift";
                        item.buttonClicked = function() {
                            sendToPurchases({ method: 'flipCard', closeAll: true });
                            sendToPurchases({
                                method: 'giftAsset',
                                itemName: root.itemName,
                                certId: root.certificateId,
                                itemType: root.itemType,
                                itemHref: root.itemHref,
                                isInstalled: root.isInstalled,
                                wornEntityID: root.wornEntityID,
                                effectImage: root.itemPreviewImageUrl
                            });
                        }
                    }
                }

                Loader {
                    id: marketplaceButton;
                    sourceComponent: contextCardButton;
                    anchors.right: giftButton.visible ? giftButton.left : parent.right;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    width: 100;

                    onLoaded: {
                        item.buttonGlyphText = hifi.glyphs.market;
                        item.itemButtonText = "View in Marketplace";
                        item.buttonClicked = function() {
                            sendToPurchases({ method: 'flipCard', closeAll: true });
                            sendToPurchases({method: 'purchases_itemInfoClicked', itemId: root.itemId});
                        }
                    }
                }

                Loader {
                    id: certificateButton;
                    sourceComponent: contextCardButton;
                    anchors.right: marketplaceButton.left;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    width: 100;

                    onLoaded: {
                        item.buttonGlyphText = hifi.glyphs.certificate;
                        item.itemButtonText = "View Certificate";
                        item.buttonClicked = function() {
                            sendToPurchases({ method: 'flipCard', closeAll: true });
                            sendToPurchases({method: 'purchases_itemCertificateClicked', itemCertificateId: root.certificateId});
                        }
                    }
                }

                Loader {
                    id: uninstallButton;
                    visible: root.isInstalled;
                    sourceComponent: contextCardButton;
                    anchors.right: certificateButton.left;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    width: 72;

                    onLoaded: {
                        item.buttonGlyphText = hifi.glyphs.uninstall;
                        item.itemButtonText = "Uninstall";
                        item.buttonClicked = function() {
                            sendToPurchases({ method: 'flipCard', closeAll: true });
                            Commerce.uninstallApp(root.itemHref);
                        }
                    }

                    onVisibleChanged: {
                        trashButton.updateProperties();
                    }
                }

                Loader {
                    id: updateButton;
                    visible: root.updateAvailable;
                    sourceComponent: contextCardButton;
                    anchors.right: uninstallButton.visible ? uninstallButton.left : certificateButton.left;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    width: 78;

                    onLoaded: {
                        item.buttonGlyphText = hifi.glyphs.update;
                        item.itemButtonText = "Update";
                        item.buttonColor = "#E2334D";
                        item.buttonClicked = function() {
                            sendToPurchases({ method: 'flipCard', closeAll: true });
                            sendToPurchases({
                                method: 'updateItemClicked',
                                itemId: root.updateAvailable ? root.updateItemId : root.itemId,
                                itemEdition: root.itemEdition,
                                itemHref: root.itemHref,
                                itemType: root.itemType,
                                isInstalled: root.isInstalled,
                                wornEntityID: root.wornEntityID
                            });
                        }
                    }

                    onVisibleChanged: {
                        trashButton.updateProperties();
                    }
                }

                Loader {
                    id: trashButton;
                    visible: root.itemEdition > 0;
                    sourceComponent: contextCardButton;
                    anchors.right: updateButton.visible ? updateButton.left : (uninstallButton.visible ? uninstallButton.left : certificateButton.left);
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    width: (updateButton.visible && uninstallButton.visible) ? 15 : 78;

                    onLoaded: {
                        item.buttonGlyphText = hifi.glyphs.trash;
                        updateProperties();
                        item.buttonClicked = function() {
                            sendToPurchases({method: 'showTrashLightbox',
                                isInstalled: root.isInstalled,
                                itemHref: root.itemHref,
                                itemName: root.itemName,
                                certID: root.certificateId,
                                itemType: root.itemType,
                                wornEntityID: root.wornEntityID
                            });
                        }
                    }

                    function updateProperties() {
                        if (updateButton.visible && uninstallButton.visible) {
                            item.itemButtonText = "";
                            item.glyphSize = 20;
                        } else if (item) {
                            item.itemButtonText = "Send to Trash";
                            item.glyphSize = 30;
                        }
                    }
                }
            }

            Rectangle {
                id: permissionExplanationCard;
                visible: false;
                anchors.left: parent.left;
                anchors.leftMargin: 30;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: closeContextMenuContainer.left;
                anchors.rightMargin: 8;
                color: hifi.colors.white;

                RalewayRegular {
                    id: permissionExplanationText;
                    anchors.fill: parent;
                    text: {
                        if (root.standaloneIncompatible) {
                            "This item is incompatible with stand-alone devices. <a href='#standaloneIncompatible'>Learn more</a>";
                        } else if (root.itemType === "contentSet") {
                            "You do not have 'Replace Content' permissions in this domain. <a href='#replaceContentPermission'>Learn more</a>";
                        } else if (root.itemType === "entity") {
                            "You do not have 'Rez Certified' permissions in this domain. <a href='#rezCertifiedPermission'>Learn more</a>";
                        } else {
                            "Hey! You're not supposed to see this. How is it even possible that you're here? Are you a developer???"
                        }
                    }
                    size: 16;
                    color: hifi.colors.baseGray;
                    wrapMode: Text.Wrap;
                    verticalAlignment: Text.AlignVCenter;

                    onLinkActivated: {
                        if (link === "#standaloneIncompatible") {
                            sendToPurchases({method: 'showStandaloneIncompatibleExplanation'});                            
                        } else {
                            sendToPurchases({method: 'showPermissionsExplanation', itemType: root.itemType});
                        }
                    }
                }
            }
        }

        transform: Rotation {
            id: rotation;
            origin.x: flipable.width/2;
            origin.y: flipable.height/2;
            axis.x: 1;
            axis.y: 0;
            axis.z: 0;
            angle: 0;
        }

        states: State {
            name: "back";
            PropertyChanges {
                target: rotation;
                angle: 180;
            }
            when: root.cardBackVisible;
        }

        transitions: Transition {
            SmoothedAnimation {
                target: rotation;
                property: "angle";
                velocity: 600;
            }
        }
    }

    Rectangle {
        id: mainContainer;
        z: 51;
        // Style
        color: hifi.colors.white;
        // Size
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;
        height: root.height - 2;

        Image {
            id: itemPreviewImage;
            source: root.itemPreviewImageUrl;
            anchors.left: parent.left;
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            width: height * 1.78;
            fillMode: Image.PreserveAspectCrop;
            mipmap: true;

            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    sendToPurchases({method: 'purchases_itemInfoClicked', itemId: root.itemId});
                }
            }
        }

        RalewayRegular {
            id: itemName;
            anchors.top: parent.top;
            anchors.topMargin: 4;
            anchors.left: itemPreviewImage.right;
            anchors.leftMargin: 10;
            anchors.right: contextMenuButtonContainer.left;
            anchors.rightMargin: 4;
            height: paintedHeight;
            // Text size
            size: 20;
            // Style
            color: hifi.colors.black;
            text: root.itemName;
            elide: Text.ElideRight;
            // Alignment
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        RalewayRegular {
            id: editionNumberText;
            visible: root.displayedItemCount > 1 && !statusContainer.visible;
            anchors.left: itemName.left;
            anchors.right: itemName.right;
            anchors.top: itemName.bottom;
            anchors.topMargin: 4;
            anchors.bottom: buttonContainer.top;
            anchors.bottomMargin: 4;
            width: itemName.width;
            text: "Edition #" + root.itemEdition;
            size: 13;
            color: hifi.colors.black;
            verticalAlignment: Text.AlignVCenter;
        }

        Item {
            id: statusContainer;
            visible: root.purchaseStatus === "pending" || !root.valid || root.numberSold > -1;
            anchors.left: itemName.left;
            anchors.right: itemName.right;
            anchors.top: itemName.bottom;
            anchors.topMargin: 4;
            anchors.bottom: buttonContainer.top;
            anchors.bottomMargin: 4;

            RalewayRegular {
                id: statusText;
                anchors.left: parent.left;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                text: {
                        if (root.purchaseStatus === "pending") {
                            "PENDING..."
                        } else if (!root.valid) {
                            "INVALIDATED"
                        } else if (root.numberSold > -1) {
                            ("Sales: " + root.numberSold + "/" + (root.limitedRun === -1 ? "\u221e" : root.limitedRun))
                        } else {
                            ""
                        }
                    }
                size: 13;
                color: {
                        if (root.purchaseStatus === "pending") {
                            hifi.colors.blueAccent
                        } else if (!root.valid) {
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
                        } else if (!root.valid) {
                            hifi.glyphs.question
                        } else {
                            ""
                        }
                    }
                // Size
                size: 34;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: -10;
                anchors.left: statusText.right;
                anchors.bottom: parent.bottom;
                // Style
                color: {
                        if (root.purchaseStatus === "pending") {
                            hifi.colors.blueAccent
                        } else if (!root.valid) {
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
                    } else if (!root.valid) {
                        sendToPurchases({method: 'showInvalidatedLightbox'});
                    }
                }
                onEntered: {
                    if (root.purchaseStatus === "pending") {
                        statusText.color = hifi.colors.blueHighlight;
                        statusIcon.color = hifi.colors.blueHighlight;
                    } else if (!root.valid) {
                        statusText.color = hifi.colors.redAccent;
                        statusIcon.color = hifi.colors.redAccent;
                    }
                }
                onExited: {
                    if (root.purchaseStatus === "pending") {
                        statusText.color = hifi.colors.blueAccent;
                        statusIcon.color = hifi.colors.blueAccent;
                    } else if (!root.valid) {
                        statusText.color = hifi.colors.redHighlight;
                        statusIcon.color = hifi.colors.redHighlight;
                    }
                }
            }
        }

        Item {
            id: contextMenuButtonContainer;
            anchors.right: parent.right;
            anchors.rightMargin: 8;
            anchors.top: parent.top;
            anchors.topMargin: 8;
            width: 30;
            height: width;

            Rectangle {
                visible: root.updateAvailable;
                anchors.fill: parent;
                radius: height;
                border.width: 1;
                border.color: "#E2334D";
            }

            HiFiGlyphs {
                id: contextMenuGlyph;
                text: hifi.glyphs.verticalEllipsis;
                anchors.fill: parent;
                size: 46;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
                color: root.updateAvailable ? "#E2334D" : hifi.colors.black;
            }

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    contextCard.visible = true;
                    permissionExplanationCard.visible = false;
                    root.sendToPurchases({ method: 'flipCard' });
                }
                onEntered: {
                    contextMenuGlyph.color = root.updateAvailable ? hifi.colors.redHighlight : hifi.colors.blueHighlight;
                }
                onExited: {
                    contextMenuGlyph.color = root.updateAvailable ? "#E2334D" : hifi.colors.black;
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
        Button {
            id: buttonContainer;
            property int color: hifi.buttons.blue;
            property int colorScheme: hifi.colorSchemes.light;

            anchors.left: itemName.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 8;
            width: 160;
            height: 40;
            enabled: !root.standaloneIncompatible && 
                root.hasPermissionToRezThis &&
                MyAvatar.skeletonModelURL !== root.itemHref &&
                !root.wornEntityID &&
                root.valid;

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
                    sendToPurchases({method: 'showReplaceContentLightbox', itemHref: root.itemHref, certID: root.certificateId, itemName: root.itemName});
                } else if (root.itemType === "avatar") {
                    sendToPurchases({method: 'showChangeAvatarLightbox', itemName: root.itemName, itemHref: root.itemHref});
                } else if (root.itemType === "app") {
                    if (root.isInstalled) {
                        Commerce.openApp(root.itemHref);
                    } else {
                        // "Run" and "Uninstall" buttons are separate.
                        Commerce.installApp(root.itemHref);
                    }
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
                    TextMetrics {
                        id: rezIconTextMetrics;
                        font: rezIcon.font;
                        text: rezIcon.text;
                    }
                    HiFiGlyphs {
                        id: rezIcon;
                        text: root.isInstalled ? "" : (root.buttonGlyph)[itemTypesArray.indexOf(root.itemType)];
                        anchors.right: rezIconLabel.left;
                        anchors.rightMargin: 2;
                        anchors.verticalCenter: parent.verticalCenter;
                        size: 36;
                        horizontalAlignment: Text.AlignHCenter;
                        color: enabled ? hifi.buttons.textColor[control.color]
                                        : hifi.buttons.disabledTextColor[control.colorScheme]
                    }
                    TextMetrics {
                        id: rezIconLabelTextMetrics;
                        font: rezIconLabel.font;
                        text: rezIconLabel.text;
                    }
                    RalewayBold {
                        id: rezIconLabel;
                        text: root.isInstalled ? "OPEN" : (MyAvatar.skeletonModelURL === root.itemHref ? "CURRENT" : (root.buttonTextNormal)[itemTypesArray.indexOf(root.itemType)]);
                        anchors.verticalCenter: parent.verticalCenter;
                        width: rezIconLabelTextMetrics.width;
                        x: parent.width/2 - rezIconLabelTextMetrics.width/2 + rezIconTextMetrics.width/2;
                        size: 15;
                        font.capitalization: Font.AllUppercase;
                        verticalAlignment: Text.AlignVCenter;
                        horizontalAlignment: Text.AlignHCenter;
                        color: enabled ? hifi.buttons.textColor[control.color]
                                        : hifi.buttons.disabledTextColor[control.colorScheme]
                    }
                }
            }
        }
        HiFiGlyphs {
            id: noPermissionGlyph;
            visible: !root.hasPermissionToRezThis;
            anchors.verticalCenter: buttonContainer.verticalCenter;
            anchors.left: buttonContainer.left;
            anchors.right: buttonContainer.right;
            anchors.rightMargin: -40;
            text: hifi.glyphs.info;
            // Size
            size: 44;
            // Style
            color: hifi.colors.redAccent;
            horizontalAlignment: Text.AlignRight;

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
                    contextCard.visible = false;
                    permissionExplanationCard.visible = true;
                    root.sendToPurchases({ method: 'flipCard' });
                }
            }
        } 
        Image {
            id: standaloneOptomizedBadge

            anchors {
                right: parent.right
                bottom: parent.bottom
                rightMargin: 15
                bottomMargin:12
            }
            height: root.standaloneOptimized ? 36 : 0
            width: 36
            
            visible: root.standaloneOptimized
            fillMode: Image.PreserveAspectFit
            source: "../../../../icons/standalone-optimized.svg"
        }
        ColorOverlay {
            anchors.fill: standaloneOptomizedBadge
            source: standaloneOptomizedBadge
            color: hifi.colors.blueHighlight
            visible: root.standaloneOptimized
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToPurchases(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
