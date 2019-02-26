//
//  ConnectionItem.qml
//  qml/hifi/commerce/common/sendAsset
//
//  ConnectionItem
//
//  Created by Zach Fox on 2018-01-09
//  Copyright 2018 High Fidelity, Inc.
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
import "../../../../controls" as HifiControls
import "../../wallet" as HifiWallet

Item {
    HifiConstants { id: hifi; }

    id: root;
    property bool isSelected: false;
    property string userName;
    property string profilePicUrl;

    height: 75;
    width: parent.width;

    Rectangle {
        id: mainContainer;
        // Style
        color: root.isSelected ? hifi.colors.faintGray80 : hifi.colors.white;
        // Size
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;
        height: root.height;
        
        Item {
            id: avatarImage;
            visible: profilePicUrl !== "" && userName !== "";
            // Size
            anchors.verticalCenter: parent.verticalCenter;
            anchors.left: parent.left;
            anchors.leftMargin: 36;
            height: 50;
            width: visible ? height : 0;
            clip: true;
            Image {
                id: userImage;
                source: root.profilePicUrl !== "" ? ((0 === root.profilePicUrl.indexOf("http")) ?
                    root.profilePicUrl : (Account.metaverseServerURL + root.profilePicUrl)) : "";
                mipmap: true;
                // Anchors
                anchors.fill: parent
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Item {
                        width: userImage.width;
                        height: userImage.height;
                        Rectangle {
                            anchors.centerIn: parent;
                            width: userImage.width; // This works because userImage is square
                            height: width;
                            radius: width;
                        }
                    }
                }
            }
            AnimatedImage {
                source: "../../../../../icons/profilePicLoading.gif"
                anchors.fill: parent;
                visible: userImage.status != Image.Ready;
            }
        }

        RalewaySemiBold {
            id: userName;
            anchors.left: avatarImage.right;
            anchors.leftMargin: 12;
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            anchors.right: chooseButton.visible ? chooseButton.left : parent.right;
            anchors.rightMargin: chooseButton.visible ? 10 : 0;
            // Text size
            size: 18;
            // Style
            color: hifi.colors.blueAccent;
            text: root.userName;
            elide: Text.ElideRight;
            // Alignment
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // "Choose" button
        HifiControlsUit.Button {
            id: chooseButton;
            visible: root.isSelected;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.dark;
            anchors.verticalCenter: parent.verticalCenter;
            anchors.right: parent.right;
            anchors.rightMargin: 28;
            height: 35;
            width: 100;
            text: "CHOOSE";
            onClicked: {
                var msg = { method: 'chooseConnection', userName: root.userName, profilePicUrl: root.profilePicUrl };
                sendToParent(msg);
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToParent(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
