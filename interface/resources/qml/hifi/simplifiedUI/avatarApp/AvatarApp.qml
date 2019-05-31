//
//  AvatarApp.qml
//
//  Created by Zach Fox on 2019-05-02
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import "../simplifiedConstants" as SimplifiedConstants
import "./components" as AvatarAppComponents
import stylesUit 1.0 as HifiStylesUit
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.

Rectangle {
    id: root

    property bool inventoryReceived: false
    property bool isDebuggingFirstUseTutorial: false
    property bool keyboardRaised: false
    property int numUpdatesAvailable: 0
    property string avatarPreviewUrl: ""

    onAvatarPreviewUrlChanged: {
        sendToScript({
            "source": "AvatarApp.qml",
            "method": "updateAvatarThumbnailURL",
            "data": {
                "avatarThumbnailURL": root.avatarPreviewUrl
            }
        });
    }

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    color: simplifiedUI.colors.darkBackground

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
            if (isLoggedIn) {
                Commerce.getWalletStatus();
            } else {
                errorText.text = "There was a problem while retrieving your inventory. " +
                    "Please try closing and re-opening the Avatar app.\n\nLogin status result: " + isLoggedIn;
            }
        }

        onWalletStatusResult: {
            if (walletStatus === 5) {
                getInventory();
            } else {
                errorText.text = "There was a problem while retrieving your inventory. " +
                    "Please try closing and re-opening the Avatar app.\n\nWallet status result: " + walletStatus;
            }
        }

        onInventoryResult: {
            if (result.status !== "success") {
                errorText.text = "There was a problem while retrieving your inventory. " +
                    "Please try closing and re-opening the Avatar app.\n\nInventory status: " + result.status + "\nMessage: " + result.message;
            } else if (result.data && result.data.assets && result.data.assets.length === 0 && avatarAppInventoryModel.count === 0) {
                errorText.text = "You have not created any avatars yet! Create an avatar with the Avatar Creator, then close and re-open the Avatar App."
            }

            avatarAppInventoryModel.handlePage(result.status !== "success" && result.message, result);
            root.updatePreviewUrl();
        }
    }

    Image {
        id: accent
        source: "../images/accent.svg"
        anchors.top: parent.top
        anchors.right: parent.right
        width: 60
        height: 103
        transform: Scale {
            yScale: -1
            origin.x: accent.width / 2
            origin.y: accent.height / 2
        }
    }

    Image {
        id: homeButton
        source: "images/homeIcon.svg"
        opacity: homeButtonMouseArea.containsMouse ? 1.0 : 0.7
        anchors.top: parent.top
        anchors.topMargin: 15
        anchors.right: parent.right
        anchors.rightMargin: 24
        width: 14
        height: 13

        MouseArea {
            id: homeButtonMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                Window.location = LocationBookmarks.getHomeLocationAddress();
            }
        }
    }

    AvatarAppComponents.DisplayNameHeader {
        id: displayNameHeader
        previewUrl: root.avatarPreviewUrl
        loading: !inventoryContentsList.visible
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 24
    }

    Item {
        id: avatarInfoTextContainer
        width: parent.implicitWidth
        height: childrenRect.height
        anchors.top: displayNameHeader.bottom
        anchors.topMargin: 30
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 24

        HifiStylesUit.GraphikRegular {
            id: yourAvatarsTitle
            text: "Your Avatars"
            anchors.top: parent.top
            verticalAlignment: TextInput.AlignVCenter
            horizontalAlignment: TextInput.AlignLeft
            color: simplifiedUI.colors.text.white
            size: 22
        }
        HifiStylesUit.GraphikRegular {
            id: yourAvatarsSubtitle
            text: "These are the avatars that you've created and uploaded via the Avatar Creator."
            width: parent.width
            wrapMode: Text.WordWrap
            anchors.top: yourAvatarsTitle.bottom
            anchors.topMargin: 6
            verticalAlignment: TextInput.AlignVCenter
            horizontalAlignment: TextInput.AlignLeft
            color: simplifiedUI.colors.text.darkGrey
            size: 14
        }
    }

    HifiModels.PSFListModel {
        id: avatarAppInventoryModel
        itemsPerPage: 4
        listModelName: 'inventory'
        listView: inventoryContentsList
        getPage: function () {
            var editionFilter = "";
            var primaryFilter = "avatar";
            var titleFilter = "";

            Commerce.inventory(
                editionFilter,
                primaryFilter,
                titleFilter,
                avatarAppInventoryModel.currentPageToRetrieve,
                avatarAppInventoryModel.itemsPerPage
            );
        }
        processPage: function(data) {
            inventoryReceived = true;
            data.assets.forEach(function (item) {
                if (item.status.length > 1) { console.warn("Unrecognized inventory status", item); }
                item.status = item.status[0];
            });
            return data.assets;
        }
    }

    Item {
        anchors.top: avatarInfoTextContainer.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
            
        AnimatedImage {
            visible: !inventoryContentsList.visible && !errorText.visible
            anchors.centerIn: parent
            width: 72
            height: width
            source: "../images/loading.gif"
        }

        ListView {
            id: inventoryContentsList
            visible: avatarAppInventoryModel.count !== 0 && !errorText.visible
            interactive: contentItem.height > height
            clip: true
            model: avatarAppInventoryModel
            anchors.fill: parent
            width: parent.width
            delegate: AvatarAppComponents.AvatarAppListDelegate {
                id: avatarAppListDelegate
                itemName: title
                itemPreviewImageUrl: preview
                itemHref: download_url
                standaloneOptimized: model.standalone_optimized
                standaloneIncompatible: model.standalone_incompatible
            }
        }

        HifiStylesUit.GraphikRegular {
            id: errorText
            text: ""
            visible: text !== ""
            anchors.fill: parent
            size: 22
            color: simplifiedUI.colors.text.white
            wrapMode: Text.Wrap 
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }


    function getInventory() {
        avatarAppInventoryModel.getFirstPage();
    }

    function updatePreviewUrl() {
        var previewUrl = "";
        var downloadUrl = "";
        for (var i = 0; i < avatarAppInventoryModel.count; ++i) {
            downloadUrl = avatarAppInventoryModel.get(i).download_url;
            previewUrl = avatarAppInventoryModel.get(i).preview;
            if (MyAvatar.skeletonModelURL === downloadUrl) {
                if (previewUrl.indexOf("missing.png") > -1) {
                    previewUrl = "../../images/defaultAvatar.svg"; // Extra `../` because the image is stored 2 levels up from `DisplayNameHeader.qml`
                }
                root.avatarPreviewUrl = previewUrl;
                return;
            }
        }
    }

    function fromScript(message) {
        switch (message.method) {
            default:
                console.log('AvatarApp.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message);
}
