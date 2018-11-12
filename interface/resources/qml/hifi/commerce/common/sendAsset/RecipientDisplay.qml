//
//  RecipientDisplay.qml
//  qml/hifi/commerce/common/sendAsset
//
//  RecipientDisplay
//
//  Created by Zach Fox on 2018-01-11
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.6
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../../controls" as HifiControls
import "../" as HifiCommerceCommon

Item {
    HifiConstants { id: hifi; }

    id: root;

    property bool isDisplayingNearby; // as opposed to 'connections'
    property string displayName;
    property string userName;
    property string profilePic;
    property string textColor: hifi.colors.white;

    Item {
        visible: root.isDisplayingNearby;
        anchors.fill: parent;

        RalewaySemiBold {
            id: recipientDisplayName;
            text: root.displayName;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.rightMargin: 12;
            height: parent.height/2;
            // Text size
            size: 18;
            // Style
            color: root.textColor;
            verticalAlignment: Text.AlignBottom;
            elide: Text.ElideRight;
        }

        RalewaySemiBold {
            text: root.userName;
            // Anchors
            anchors.bottom: parent.bottom;
            anchors.left: recipientDisplayName.anchors.left;
            anchors.leftMargin: recipientDisplayName.anchors.leftMargin;
            anchors.right: recipientDisplayName.anchors.right;
            anchors.rightMargin: recipientDisplayName.anchors.rightMargin;
            height: parent.height/2;
            // Text size
            size: 16;
            // Style
            color: root.textColor;
            verticalAlignment: Text.AlignTop;
            elide: Text.ElideRight;
        }
    }

    Item {
        visible: !root.isDisplayingNearby;
        anchors.fill: parent;

        Image {
            id: userImage;
            source: root.profilePic;
            mipmap: true;
            // Anchors
            anchors.left: parent.left;
            anchors.verticalCenter: parent.verticalCenter;
            height: parent.height - 36;
            width: height;
            layer.enabled: true;
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

        RalewaySemiBold {
            text: root.userName;
            // Anchors
            anchors.left: userImage.right;
            anchors.leftMargin: 8;
            anchors.right: parent.right;
            anchors.verticalCenter: parent.verticalCenter;
            height: parent.height - 4;
            // Text size
            size: 16;
            // Style
            color: root.textColor;
            verticalAlignment: Text.AlignVCenter;
            elide: Text.ElideRight;
        }
    }
}
