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
                // Show some error to the user
            }
        }

        onWalletStatusResult: {
            if (walletStatus === 5) {
                getInventory();
            } else {
                // Show some error to the user
            }
        }

        onInventoryResult: {
            inventoryModel.handlePage(result.status !== "success" && result.message, result);
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

    AvatarAppComponents.DisplayNameHeader {
        id: displayNameHeader
        previewUrl: avatarPreviewUrl
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
        id: inventoryModel
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
                inventoryModel.currentPageToRetrieve,
                inventoryModel.itemsPerPage
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
            visible: !inventoryContentsList.visible
            anchors.centerIn: parent
            width: 72
            height: width
            source: "../images/loading.gif"
        }

        ListView {
            id: inventoryContentsList
            visible: inventoryModel.count !== 0
            interactive: contentItem.height > height
            clip: true
            model: inventoryModel
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
    }


    function getInventory() {
        inventoryModel.getFirstPage();
    }

    function updatePreviewUrl() {
        var previewUrl = "";
        var downloadUrl = "";
        for (var i = 0; i < inventoryModel.count; ++i) {
            downloadUrl = inventoryModel.get(i).download_url;
            previewUrl = inventoryModel.get(i).preview;
            if (MyAvatar.skeletonModelURL === downloadUrl) {
                avatarPreviewUrl = previewUrl;
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
