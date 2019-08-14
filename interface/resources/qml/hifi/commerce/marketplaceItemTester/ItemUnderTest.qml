//
//  ItemUnderTest
//  qml/hifi/commerce/marketplaceItemTester
//
//  Load items not in the marketplace for testing purposes
//
//  Created by Kerry Ivan Kurian on 2018-10-18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import Hifi 1.0 as Hifi
import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HifiControlsUit

Rectangle {
    id: root;
    color: hifi.colors.baseGray
    width: parent.width - 16
    height: childrenRect.height + itemHeaderContainer.anchors.topMargin + detailsContainer.anchors.topMargin

    property var detailsExpanded: false

    property var actions: {
        "forward": function(resource, assetType, resourceObjectId){
            switch(assetType) {
                case "application":
                    Commerce.installApp(resource, true);
                    break;
                case "avatar":
                    MyAvatar.useFullAvatarURL(resource);
                    break;
                case "content set":
                    urlHandler.handleUrl("hifi://localhost/0,0,0");
                    Commerce.replaceContentSet(toUrl(resource), "", "");
                    break;
                case "entity":
                case "wearable":
                    rezEntity(resource, assetType, resourceObjectId);
                    break;
                default:
                    print("Marketplace item tester unsupported assetType " + assetType);
            }
        },
        "trash": function(resource, assetType){
            if ("application" === assetType) {
                Commerce.uninstallApp(resource);
            }
            sendToScript({
                method: "tester_deleteResourceObject",
                objectId: resourceListModel.get(index).resourceObjectId});
            resourceListModel.remove(index);
        }
    }

    Item {
        id: itemHeaderContainer
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8
        width: parent.width - 16
        height: childrenRect.height

        Item {
            id: itemNameContainer
            width: parent.width * 0.5
            height: childrenRect.height

            HifiStylesUit.RalewaySemiBold {
                id: resourceName
                height: paintedHeight
                width: parent.width
                text: {
                    var match = resource.match(/\/([^/]*)$/);
                    return match ? match[1] : resource;
                }
                size: 14
                color: hifi.colors.white
                wrapMode: Text.WrapAnywhere
            }

            HifiStylesUit.RalewayRegular {
                id: resourceUrl
                anchors.top: resourceName.bottom;
                anchors.topMargin: 4;
                height: paintedHeight
                width: parent.width
                text: resource
                size: 12
                color: hifi.colors.faintGray;
                wrapMode: Text.WrapAnywhere
            }
        }

        HifiControlsUit.ComboBox {
            id: comboBox
            anchors.left: itemNameContainer.right
            anchors.leftMargin: 4
            anchors.verticalCenter: itemNameContainer.verticalCenter
            height: 30
            width: parent.width * 0.3 - anchors.leftMargin

            model: [
                "application",
                "avatar",
                "content set",
                "entity",
                "wearable",
                "unknown"
            ]

            currentIndex: (("entity or wearable" === assetType) ?
                model.indexOf("unknown") : model.indexOf(assetType))

            Component.onCompleted: {
                onCurrentIndexChanged.connect(function() {
                    assetType = model[currentIndex];
                    sendToScript({
                        method: "tester_updateResourceObjectAssetType",
                        objectId: resourceListModel.get(index)["resourceObjectId"],
                        assetType: assetType });
                });
            }
        }

        Button {
            id: actionButton
            property var glyphs: {
                "application": hifi.glyphs.install,
                "avatar": hifi.glyphs.avatar,
                "content set": hifi.glyphs.globe,
                "entity": hifi.glyphs.wand,
                "trash": hifi.glyphs.trash,
                "unknown": hifi.glyphs.circleSlash,
                "wearable": hifi.glyphs.hat
            }
            property int color: hifi.buttons.blue;
            property int colorScheme: hifi.colorSchemes.dark;
            anchors.left: comboBox.right
            anchors.leftMargin: 4
            anchors.verticalCenter: itemNameContainer.verticalCenter
            width: parent.width * 0.10 - anchors.leftMargin
            height: width
            enabled: comboBox.model[comboBox.currentIndex] !== "unknown"

            onClicked: {
                if (model.currentlyRecordingResources) {
                    model.currentlyRecordingResources = false;
                } else {
                    model.resourceAccessEventText = "";
                    model.currentlyRecordingResources = true;
                    root.actions["forward"](resource, comboBox.currentText, resourceObjectId);
                }
                sendToScript({
                    method: "tester_updateResourceRecordingStatus",
                    objectId: resourceListModel.get(index).resourceObjectId,
                    status: model.currentlyRecordingResources
                });
            }

            background: Rectangle {
                radius: 4;
                gradient: Gradient {
                    GradientStop {
                        position: 0.2
                        color: {
                            if (!actionButton.enabled) {
                                hifi.buttons.disabledColorStart[actionButton.colorScheme]
                            } else if (actionButton.pressed) {
                                hifi.buttons.pressedColor[actionButton.color]
                            } else if (actionButton.hovered) {
                                hifi.buttons.hoveredColor[actionButton.color]
                            } else {
                                hifi.buttons.colorStart[actionButton.color]
                            }
                        }
                    }
                    GradientStop {
                        position: 1.0
                        color: {
                            if (!actionButton.enabled) {
                                hifi.buttons.disabledColorFinish[actionButton.colorScheme]
                            } else if (actionButton.pressed) {
                                hifi.buttons.pressedColor[actionButton.color]
                            } else if (actionButton.hovered) {
                                hifi.buttons.hoveredColor[actionButton.color]
                            } else {
                                hifi.buttons.colorFinish[actionButton.color]
                            }
                        }
                    }
                }
            }

            contentItem: Item {
                HifiStylesUit.HiFiGlyphs {
                    id: rezIcon;
                    text: model.currentlyRecordingResources ? hifi.glyphs.scriptStop : actionButton.glyphs[comboBox.model[comboBox.currentIndex]];
                    anchors.fill: parent
                    size: 30;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    color: enabled ? hifi.buttons.textColor[actionButton.color]
                                    : hifi.buttons.disabledTextColor[actionButton.colorScheme]
                }
            }
        }

        Button {
            id: trashButton
            property int color: hifi.buttons.red;
            property int colorScheme: hifi.colorSchemes.dark;
            anchors.left: actionButton.right
            anchors.verticalCenter: itemNameContainer.verticalCenter
            anchors.leftMargin: 4
            width: parent.width * 0.10 - anchors.leftMargin
            height: width

            onClicked: {
                root.actions["trash"](resource, comboBox.currentText, resourceObjectId);
            }

            background: Rectangle {
                radius: 4;
                gradient: Gradient {
                    GradientStop {
                        position: 0.2
                        color: {
                            if (!trashButton.enabled) {
                                hifi.buttons.disabledColorStart[trashButton.colorScheme]
                            } else if (trashButton.pressed) {
                                hifi.buttons.pressedColor[trashButton.color]
                            } else if (trashButton.hovered) {
                                hifi.buttons.hoveredColor[trashButton.color]
                            } else {
                                hifi.buttons.colorStart[trashButton.color]
                            }
                        }
                    }
                    GradientStop {
                        position: 1.0
                        color: {
                            if (!trashButton.enabled) {
                                hifi.buttons.disabledColorFinish[trashButton.colorScheme]
                            } else if (trashButton.pressed) {
                                hifi.buttons.pressedColor[trashButton.color]
                            } else if (trashButton.hovered) {
                                hifi.buttons.hoveredColor[trashButton.color]
                            } else {
                                hifi.buttons.colorFinish[trashButton.color]
                            }
                        }
                    }
                }
            }

            contentItem: Item {
                HifiStylesUit.HiFiGlyphs {
                    id: trashIcon;
                    text: hifi.glyphs.trash
                    anchors.fill: parent
                    size: 22;
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: enabled ? hifi.buttons.textColor[trashButton.color]
                                    : hifi.buttons.disabledTextColor[trashButton.colorScheme]
                }
            }
        }
    }

    Item {
        id: detailsContainer

        width: parent.width - 16
        height: root.detailsExpanded ? 300 : 26
        anchors.top: itemHeaderContainer.bottom
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.leftMargin: 8

        HifiStylesUit.HiFiGlyphs {
            id: detailsToggle
            anchors.left: parent.left
            anchors.leftMargin: -4
            anchors.top: parent.top
            anchors.topMargin: -2
            width: 22
            text: root.detailsExpanded ? hifi.glyphs.minimize : hifi.glyphs.maximize
            color: hifi.colors.white
            size: 22
            MouseArea {
                anchors.fill: parent
                onClicked: root.detailsExpanded = !root.detailsExpanded
            }
        }

        ScrollView {
            id: detailsTextContainer
            anchors.top: parent.top
            anchors.left: detailsToggle.right
            anchors.leftMargin: 4
            anchors.right: parent.right
            height: detailsContainer.height - (root.detailsExpanded ? (copyToClipboardButton.height + copyToClipboardButton.anchors.topMargin) : 0)
            clip: true

            TextArea {
                id: detailsText
                readOnly: true
                color: hifi.colors.white
                text: {
                    var numUniqueResources = (model.resourceAccessEventText.split("\n").length - 1);
                    if (root.detailsExpanded && numUniqueResources > 0) {
                        return model.resourceAccessEventText
                    } else {
                        return numUniqueResources.toString() + " unique source/resource url pair" + (numUniqueResources === 1 ? "" : "s") + " recorded"
                    }
                }
                font: Qt.font({ family: "Courier", pointSize: 8, weight: Font.Normal })
                wrapMode: TextEdit.NoWrap

                background: Rectangle {
                    anchors.fill: parent;
                    color: hifi.colors.baseGrayShadow;
                    border.width: 0;
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (root.detailsExpanded) {
                        detailsText.selectAll();
                    } else {
                        root.detailsExpanded = true;
                    }
                }
            }
        }

        HifiControlsUit.Button {
            id: copyToClipboardButton;
            visible: root.detailsExpanded
            color: hifi.buttons.noneBorderlessWhite
            colorScheme: hifi.colorSchemes.dark

            anchors.top: detailsTextContainer.bottom
            anchors.topMargin: 8
            anchors.right: parent.right
            width: 160
            height: 30
            text: "Copy to Clipboard"

            onClicked: {
                Window.copyToClipboard(detailsText.text);
            }
        }
    }
}
