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
import hifi.simplifiedUI.simplifiedControls 1.0 as SimplifiedControls
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
        var numTimesRun = Settings.getValue("simplifiedUI/SUIScriptExecutionCount", 0);
        numTimesRun++;
        Settings.setValue("simplifiedUI/SUIScriptExecutionCount", numTimesRun);
        Commerce.getLoginStatus();
    }

    Connections {
        target: MyAvatar

        onSkeletonModelURLChanged: {
            root.updatePreviewUrl();

            if ((MyAvatar.skeletonModelURL.indexOf("defaultAvatar") > -1 || MyAvatar.skeletonModelURL.indexOf("fst") === -1) &&
                topBarInventoryModel.count > 0) {
                Settings.setValue("simplifiedUI/alreadyAutoSelectedAvatarFromInventory", true);
                MyAvatar.useFullAvatarURL(topBarInventoryModel.get(0).download_url);
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
                // Show some error to the user in the UI?
            }
        }

        onWalletStatusResult: {
            if (inventoryFullyReceived) {
                return;
            }

            switch (walletStatus) {
                case 1:
                    var randomNumber = Math.floor(Math.random() * 34) + 1;
                    var securityImagePath = "images/" + randomNumber.toString().padStart(2, '0') + ".jpg";
                    Commerce.getWalletAuthenticatedStatus(); // before writing security image, ensures that salt/account password is set.
                    Commerce.chooseSecurityImage(securityImagePath);
                    Commerce.generateKeyPair()
                    Commerce.getWalletStatus();
                    break;
                case 5:
                    topBarInventoryModel.getFirstPage();
                    break;
                default:
                    console.log('WARNING: SimplifiedTopBar.qml walletStatusResult:', walletStatus);
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
                var scriptExecutionCount = Settings.getValue("simplifiedUI/SUIScriptExecutionCount");
                var currentAvatarURL = MyAvatar.skeletonModelURL;
                var currentAvatarURLContainsDefaultAvatar = currentAvatarURL.indexOf("defaultAvatar") > -1;
                var currentAvatarURLContainsFST = currentAvatarURL.indexOf("fst") > -1;
                var currentAvatarURLContainsSimplifiedAvatar = currentAvatarURL.indexOf("simplifiedAvatar") > -1;
                var alreadyAutoSelectedAvatarFromInventory = Settings.getValue("simplifiedUI/alreadyAutoSelectedAvatarFromInventory", false);
                var userHasValidAvatarInInventory = topBarInventoryModel.count > 0 && 
                    topBarInventoryModel.get(0).download_url.indexOf(".fst") > -1;
                var simplifiedAvatarPrefix = "https://content.highfidelity.com/Experiences/Releases/simplifiedUI/simplifiedFTUE/avatars/simplifiedAvatar_";
                var simplifiedAvatarColors = ["Blue", "Cyan", "Green", "Magenta", "Red"];
                var simplifiedAvatarSuffix = "/avatar.fst";

                // Use `Settings.setValue("simplifiedUI/debugFTUE", 0);` to turn off FTUE Debug Mode.
                // Use `Settings.setValue("simplifiedUI/debugFTUE", 1);` to debug FTUE Screen 1.
                // Use `Settings.setValue("simplifiedUI/debugFTUE", 2);` to debug FTUE Screen 2.
                // Use `Settings.setValue("simplifiedUI/debugFTUE", 3);` to debug FTUE Screen 3.
                // Use `Settings.setValue("simplifiedUI/debugFTUE", 4);` to force the UI to show what would happen if the user had an empty Inventory.

                var debugFTUE = Settings.getValue("simplifiedUI/debugFTUE", 0);
                if (debugFTUE === 1 || debugFTUE === 2) {
                    scriptExecutionCount = 1;
                    currentAvatarURLContainsDefaultAvatar = true;
                    if (debugFTUE === 1) {
                        userHasValidAvatarInInventory = false;
                        currentAvatarURLContainsSimplifiedAvatar = false;
                    }
                } else if (debugFTUE === 3) {
                    scriptExecutionCount = 2;
                    currentAvatarURLContainsDefaultAvatar = false;
                    currentAvatarURLContainsSimplifiedAvatar = true;
                }

                // If we have never auto-selected and the user is still using a default avatar or if the current avatar is not valid (fst), or if 
                // the current avatar is the old default (Woody), use top avatar from inventory or one of the new defaults.

                // If the current avatar URL is invalid, OR the user is using the "default avatar" (Woody)...
                if (!currentAvatarURLContainsFST || currentAvatarURLContainsDefaultAvatar) {
                    // If the user has a valid avatar in their inventory...
                    if (userHasValidAvatarInInventory) {
                        // ...use the first avatar in the user's inventory.
                        MyAvatar.useFullAvatarURL(topBarInventoryModel.get(0).download_url);
                        Settings.setValue("simplifiedUI/alreadyAutoSelectedAvatarFromInventory", true);
                    // Else if the user isn't wearing a "Simplified Avatar"
                    } else if (!currentAvatarURLContainsSimplifiedAvatar) {
                        // ...assign to the user a new "Simplified Avatar" (i.e. a simple avatar of random color)
                        var avatarColor = simplifiedAvatarColors[Math.floor(Math.random() * simplifiedAvatarColors.length)];
                        var simplifiedAvatarModelURL = simplifiedAvatarPrefix + avatarColor + simplifiedAvatarSuffix;
                        MyAvatar.useFullAvatarURL(simplifiedAvatarModelURL);
                        currentAvatarURLContainsSimplifiedAvatar = true;
                    }
                }

                if (scriptExecutionCount === 1) {
                    sendToScript({
                        "source": "SimplifiedTopBar.qml",
                        "method": "displayInitialLaunchWindow"
                    });
                } else if (scriptExecutionCount === 2 && currentAvatarURLContainsSimplifiedAvatar) {
                    sendToScript({
                        "source": "SimplifiedTopBar.qml",
                        "method": "displaySecondLaunchWindow"
                    });
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
            source: "../images/defaultAvatar.svg"
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
        visible: false // An experiment. Will you notice?
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
            mipmap: true
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
        visible: false // An experiment. Will you notice?
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
            mipmap: true
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


    TextMetrics {
        id: goToTextFieldMetrics
        font: goToTextField.font
        text: goToTextField.longPlaceholderText
    }


    Item {
        id: goToTextFieldContainer
        anchors.left: statusButtonContainer.right
        anchors.leftMargin: 12
        anchors.right: (hmdButtonContainer.visible ? hmdButtonContainer.left : helpButtonContainer.left)
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height

        SimplifiedControls.TextField {
            id: goToTextField
            readonly property string shortPlaceholderText: "Jump to..."
            readonly property string longPlaceholderText: "Quickly jump to a location by typing '/LocationName'"
            anchors.centerIn: parent
            width: Math.min(parent.width, 445)
            height: 35
            leftPadding: 8
            rightPadding: 8
            bottomBorderVisible: false
            backgroundColor: "#1D1D1D"
            placeholderTextColor: "#8E8E8E"
            font.pixelSize: 14
            placeholderText: width - leftPadding - rightPadding < goToTextFieldMetrics.width ? shortPlaceholderText : longPlaceholderText
            blankPlaceholderTextOnFocus: false
            clip: true
            selectByMouse: true
            autoScroll: true
            onAccepted: {
                if (goToTextField.length > 0) {
                    AddressManager.handleLookupString(goToTextField.text);
                    goToTextField.text = "";
                }
                parent.forceActiveFocus();
            }
        }
    }



    Item {
        id: hmdButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: helpButtonContainer.left
        anchors.rightMargin: 3
        width: 48
        height: width
        visible: false

        Image {
            id: displayModeImage
            source: HMD.active ? "images/desktopMode.svg" : "images/vrMode.svg"
            anchors.centerIn: parent
            width: HMD.active ? 25 : 26
            height: HMD.active ? 22 : 14
            visible: false
            mipmap: true
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
        id: helpButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: settingsButtonContainer.left
        anchors.rightMargin: 4
        width: 36
        height: width

        Image {
            id: helpButtonImage
            source: "images/questionMark.svg"
            anchors.centerIn: parent
            width: 13
            height: 22
            visible: false
            mipmap: true
        }

        ColorOverlay {
            opacity: helpButtonMouseArea.containsMouse ? 1.0 : 0.7
            anchors.fill: helpButtonImage
            source: helpButtonImage
            color: simplifiedUI.colors.text.white
        }

        MouseArea {
            id: helpButtonMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onEntered: {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                sendToScript({
                    "source": "SimplifiedTopBar.qml",
                    "method": "toggleHelpApp"
                });
            }
        }
    }



    Item {
        id: settingsButtonContainer
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 3
        width: 36
        height: width

        Image {
            id: settingsButtonImage
            source: "images/settings.svg"
            anchors.centerIn: parent
            width: 22
            height: 22
            visible: false
            mipmap: true
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
        
        avatarButtonImage.source = "../images/defaultAvatar.svg";
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
