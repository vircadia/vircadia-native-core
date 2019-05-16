//
//  SimplifiedTopBar.qml
//
//  Created by Zach Fox on 2019-05-02
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import "../simplifiedConstants" as SimplifiedConstants
import "../inputDeviceButton" as InputDeviceButton
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import QtGraphicalEffects 1.0
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.

Rectangle {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent

    property bool inventoryFullyReceived: false

    Component.onCompleted: {
        Commerce.getLoginStatus();
    }

    Connections {
        target: MyAvatar

        onSkeletonModelURLChanged: {
            root.updatePreviewUrl();
        }
    }

    Connections {
        target: Commerce

        onLoginStatusResult: {
            if (inventoryFullyReceived) {
                return;
            }
            
            if (isLoggedIn) {
                Commerce.getWalletStatus();
            } else {
                // Show some error to the user
            }
        }

        onWalletStatusResult: {
            if (inventoryFullyReceived) {
                return;
            }

            if (walletStatus === 5) {
                topBarInventoryModel.getFirstPage();
            } else {
                // Show some error to the user
            }
        }

        onInventoryResult: {
            if (inventoryFullyReceived) {
                return;
            }

            topBarInventoryModel.handlePage(result.status !== "success" && result.message, result);
            root.updatePreviewUrl();

            // I _should_ be able to do `if (currentPageToRetrieve > -1)` here, but the
            // inventory endpoint doesn't return `response.total_pages`, so the PSFListModel doesn't
            // know when to automatically stop fetching new pages.
            // This will result in fetching one extra page than is necessary, but that's not a big deal.
            if (result.data.assets.length > 0) {
                topBarInventoryModel.getNextPage();
            } else {
                inventoryFullyReceived = true;
            }
        }
    }

    HifiModels.PSFListModel {
        id: topBarInventoryModel
        itemsPerPage: 8
        listModelName: 'inventory'
        getPage: function () {
            var editionFilter = "";
            var primaryFilter = "avatar";
            var titleFilter = "";

            Commerce.inventory(
                editionFilter,
                primaryFilter,
                titleFilter,
                topBarInventoryModel.currentPageToRetrieve,
                topBarInventoryModel.itemsPerPage
            );
        }
        processPage: function(data) {
            data.assets.forEach(function (item) {
                if (item.status.length > 1) { console.warn("Unrecognized inventory status", item); }
                item.status = item.status[0];
            });
            return data.assets;
        }
    }


    Item {
        id: avatarButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 16
        width: 48
        height: width

        AnimatedImage {
            visible: avatarButtonImage.source === ""
            anchors.centerIn: parent
            width: parent.width - 10
            height: width
            source: "../images/loading.gif"
        }

        Image {
            id: avatarButtonImage
            visible: source !== ""
            source: ""
            anchors.centerIn: parent
            width: parent.width - 10
            height: width
            mipmap: true
            fillMode: Image.PreserveAspectCrop
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: mask
            }

            MouseArea {
                id: avatarButtonImageMouseArea
                anchors.fill: parent
                hoverEnabled: enabled
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    
                    if (Account.loggedIn) {
                        sendToScript({
                            "source": "SimplifiedTopBar.qml",
                            "method": "toggleAvatarApp"
                        });
                    } else {
                        DialogsManager.showLoginDialog();
                    }
                }
            }
        }

        Rectangle {
            z: -1
            id: borderMask
            width: avatarButtonImageMouseArea.containsMouse ? avatarButtonImage.width + 4 : avatarButtonImage.width - 4
            height: width
            radius: width
            anchors.centerIn: avatarButtonImage
            color: "#FFFFFF"

            Behavior on width {
                enabled: true
                SmoothedAnimation { velocity: 80 }
            }
        }

        Rectangle {
            id: mask
            anchors.fill: avatarButtonImage
            radius: avatarButtonImage.width
            visible: false
        }
    }


    InputDeviceButton.InputDeviceButton {
        id: inputDeviceButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: avatarButtonContainer.right
        anchors.leftMargin: 8
    }


    Item {
        id: outputDeviceButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: inputDeviceButton.right
        anchors.leftMargin: 24
        width: 20
        height: width

        HifiStylesUit.HiFiGlyphs {
            property bool outputMuted: false
            id: outputDeviceButton
            text: (outputDeviceButton.outputMuted ? simplifiedUI.glyphs.vol_0 : simplifiedUI.glyphs.vol_3)
            color: (outputDeviceButton.outputMuted ? simplifiedUI.colors.controls.outputVolumeButton.text.muted : simplifiedUI.colors.controls.outputVolumeButton.text.noisy)
            opacity: outputDeviceButtonMouseArea.containsMouse ? 1.0 : 0.7
            size: 32
            anchors.centerIn: parent
            width: parent.width
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                id: outputDeviceButtonMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    outputDeviceButton.outputMuted = !outputDeviceButton.outputMuted;

                    sendToScript({
                        "source": "SimplifiedTopBar.qml",
                        "method": "setOutputMuted",
                        "data": {
                            "outputMuted": outputDeviceButton.outputMuted
                        }
                    });
                }
            }
        }
    }



    Item {
        id: hmdButtonContainer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: settingsButtonContainer.left
        anchors.rightMargin: 8
        width: height

        HifiStylesUit.HiFiGlyphs {
            id: hmdGlyph
            text: HMD.active ? simplifiedUI.glyphs.hmd : simplifiedUI.glyphs.screen
            color: simplifiedUI.colors.text.white
            opacity: hmdGlyphMouseArea.containsMouse ? 1.0 : 0.7
            size: 40
            anchors.centerIn: parent
            width: 35
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                id: hmdGlyphMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    // TODO: actually do this right and change the display plugin
                    HMD.active = !HMD.active;
                }
            }
        }
    }



    Item {
        id: settingsButtonContainer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16
        width: height

        HifiStylesUit.HiFiGlyphs {
            id: settingsGlyph
            text: simplifiedUI.glyphs.gear
            color: simplifiedUI.colors.text.white
            opacity: settingsGlyphMouseArea.containsMouse ? 1.0 : 0.7
            size: 40
            anchors.centerIn: parent
            width: 35
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                id: settingsGlyphMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "SimplifiedTopBar.qml",
                        "method": "toggleSettingsApp"
                    });
                }
            }
        }
    }


    function updatePreviewUrl() {
        var previewUrl = "";
        var downloadUrl = "";
        for (var i = 0; i < topBarInventoryModel.count; ++i) {
            downloadUrl = topBarInventoryModel.get(i).download_url;
            previewUrl = topBarInventoryModel.get(i).preview;
            if (MyAvatar.skeletonModelURL === downloadUrl) {
                avatarButtonImage.source = previewUrl;
                return;
            }
        }
    }


    function fromScript(message) {
        if (message.source !== "simplifiedUI.js") {
            return;
        }

        switch (message.method) {
            case "updateAvatarThumbnailURL":
                avatarButtonImage.source = message.data.avatarThumbnailURL;
                break;

            case "updateOutputMuted":
                outputDeviceButton.outputMuted = message.data.outputMuted;
                break;

            default:
                console.log('SimplifiedTopBar.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message);
}
