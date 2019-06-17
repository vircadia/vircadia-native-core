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
    focus: true
    
    signal keyPressEvent(int key, int modifiers)
    Keys.onPressed: {
        keyPressEvent(event.key, event.modifiers);
    }
    signal keyReleaseEvent(int key, int modifiers)
    Keys.onReleased: {
        keyReleaseEvent(event.key, event.modifiers);
    }

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

            if ((MyAvatar.skeletonModelURL.indexOf("defaultAvatar") > -1 || MyAvatar.skeletonModelURL.indexOf("fst") === -1) &&
                topBarInventoryModel.count > 0) {
                Settings.setValue("simplifiedUI/alreadyAutoSelectedAvatar", true);
                MyAvatar.skeletonModelURL = topBarInventoryModel.get(0).download_url;
            }
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

                // If we have an avatar in our inventory AND we haven't already auto-selected an avatar...
                if ((!Settings.getValue("simplifiedUI/alreadyAutoSelectedAvatar", false) ||
                    MyAvatar.skeletonModelURL.indexOf("defaultAvatar") > -1 || MyAvatar.skeletonModelURL.indexOf("fst") === -1) && topBarInventoryModel.count > 0) {
                    Settings.setValue("simplifiedUI/alreadyAutoSelectedAvatar", true);
                    MyAvatar.skeletonModelURL = topBarInventoryModel.get(0).download_url;
                }
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
        anchors.leftMargin: 2
        width: 48
        height: width

        Image {
            id: avatarButtonImage
            source: "./images/defaultAvatar.svg"
            anchors.centerIn: parent
            width: 32
            height: width
            sourceSize.width: width
            sourceSize.height: height
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
        anchors.leftMargin: 2
        width: 32
        height: width
    }


    Item {
        id: outputDeviceButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: inputDeviceButton.right
        anchors.leftMargin: 7
        width: 32
        height: width

        Image {
            id: outputDeviceButton
            property bool outputMuted: AudioScriptingInterface.avatarGain === simplifiedUI.numericConstants.mutedValue &&
                AudioScriptingInterface.serverInjectorGain === simplifiedUI.numericConstants.mutedValue &&
                AudioScriptingInterface.localInjectorGain === simplifiedUI.numericConstants.mutedValue &&
                AudioScriptingInterface.systemInjectorGain === simplifiedUI.numericConstants.mutedValue
            source: outputDeviceButton.outputMuted ? "./images/outputDeviceMuted.svg" : "./images/outputDeviceLoud.svg"
            anchors.centerIn: parent
            width: outputDeviceButton.outputMuted ? 25 : 26
            height: 22
            visible: false
        }

        ColorOverlay {
            anchors.fill: outputDeviceButton
            opacity: outputDeviceButtonMouseArea.containsMouse ? 1.0 : 0.7
            source: outputDeviceButton
            color: (outputDeviceButton.outputMuted ? simplifiedUI.colors.controls.outputVolumeButton.text.muted : simplifiedUI.colors.controls.outputVolumeButton.text.noisy)
        }

        MouseArea {
            id: outputDeviceButtonMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onEntered: {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);

                if (!outputDeviceButton.outputMuted && !AudioScriptingInterface.muted) {
                    AudioScriptingInterface.muted = true;
                }

                sendToScript({
                    "source": "SimplifiedTopBar.qml",
                    "method": "setOutputMuted",
                    "data": {
                        "outputMuted": !outputDeviceButton.outputMuted
                    }
                });
            }
        }
    }


    Item {
        id: statusButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: outputDeviceButtonContainer.right
        anchors.leftMargin: 8
        width: 36
        height: width

        Rectangle {
            id: statusButton
            property string currentStatus
            anchors.centerIn: parent
            width: 22
            height: width
            radius: width/2
            visible: false
        }

        ColorOverlay {
            anchors.fill: statusButton
            opacity: statusButton.currentStatus ? (statusButtonMouseArea.containsMouse ? 1.0 : 0.7) : 0.7
            source: statusButton
            color: if (statusButton.currentStatus === "busy") {
                "#ff001a"
            } else if (statusButton.currentStatus === "available") {
                "#009036"
            } else if (statusButton.currentStatus) {
                "#ffed00"
            } else {
                "#7e8c81"
            }
        }

        Image {
            id: statusIcon
            source: statusButton.currentStatus === "available" ? "images/statusPresent.svg" : "images/statusAway.svg"
            anchors.centerIn: parent
            width: statusButton.currentStatus === "busy" ? 13 : 14
            height: statusButton.currentStatus === "busy" ? 2 : 10
        }

        ColorOverlay {
            anchors.fill: statusIcon
            opacity: statusButton.currentStatus ? (statusButtonMouseArea.containsMouse ? 1.0 : 0.7) : 0.7
            source: statusIcon
            color: "#ffffff"
        }

        MouseArea {
            id: statusButtonMouseArea
            anchors.fill: parent
            enabled: statusButton.currentStatus
            hoverEnabled: true
            onEntered: {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);

                sendToScript({
                    "source": "SimplifiedTopBar.qml",
                    "method": "toggleStatus"
                });
            }
        }
    }



    Item {
        id: hmdButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: settingsButtonContainer.left
        anchors.rightMargin: 8
        width: 48
        height: width
        visible: false

        Image {
            id: displayModeImage
            source: HMD.active ? "./images/desktopMode.svg" : "./images/vrMode.svg"
            anchors.centerIn: parent
            width: HMD.active ? 25 : 43
            height: 22
            visible: false
        }

        ColorOverlay {
            anchors.fill: displayModeImage
            opacity: displayModeMouseArea.containsMouse ? 1.0 : 0.7
            source: displayModeImage
            color: simplifiedUI.colors.text.white
        }

        MouseArea {
            id: displayModeMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onEntered: {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                var displayPluginCount = Window.getDisplayPluginCount();
                if (HMD.active) {
                    // Switch to desktop mode - selects first VR display plugin
                    for (var i = 0; i < displayPluginCount; i++) {
                        if (!Window.isDisplayPluginHmd(i)) {
                            Window.setActiveDisplayPlugin(i);
                            return;
                        }
                    }
                } else {
                    // Switch to VR mode - selects first HMD display plugin
                    for (var i = 0; i < displayPluginCount; i++) {
                        if (Window.isDisplayPluginHmd(i)) {
                            Window.setActiveDisplayPlugin(i);
                            return;
                        }
                    }
                }
            }

            Component.onCompleted: {
                // Don't show VR button unless they have a VR headset.
                var displayPluginCount = Window.getDisplayPluginCount();
                for (var i = 0; i < displayPluginCount; i++) {
                    if (Window.isDisplayPluginHmd(i)) {
                        hmdButtonContainer.visible = true;
                        return;
                    }
                }
            }
        }
    }



    Item {
        id: settingsButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 2
        width: 48
        height: width

        Image {
            id: settingsButtonImage
            source: "./images/settings.svg"
            anchors.centerIn: parent
            width: 22
            height: 22
            fillMode: Image.PreserveAspectFit
            visible: false
        }

        ColorOverlay {
            opacity: settingsButtonMouseArea.containsMouse ? 1.0 : 0.7
            anchors.fill: settingsButtonImage
            source: settingsButtonImage
            color: simplifiedUI.colors.text.white
        }

        MouseArea {
            id: settingsButtonMouseArea
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


    function updatePreviewUrl() {
        var previewUrl = "";
        var downloadUrl = "";
        for (var i = 0; i < topBarInventoryModel.count; ++i) {
            downloadUrl = topBarInventoryModel.get(i).download_url;
            previewUrl = topBarInventoryModel.get(i).preview;
            if (MyAvatar.skeletonModelURL === downloadUrl) {
                if (previewUrl.indexOf("missing.png") > -1) {
                    previewUrl = "../images/defaultAvatar.svg";
                }
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
                if (message.data.avatarThumbnailURL.indexOf("defaultAvatar.svg") > -1) {
                    avatarButtonImage.source = "../images/defaultAvatar.svg";
                } else {
                    avatarButtonImage.source = message.data.avatarThumbnailURL;
                }
                break;

            case "updateStatusButton":
                statusButton.currentStatus = message.data.currentStatus;
                break;

            default:
                console.log('SimplifiedTopBar.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message)
}
