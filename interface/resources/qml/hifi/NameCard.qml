//
//  NameCard.qml
//  qml/hifi
//
//  Created by Howard Stearns on 12/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../styles-uit"

Row {
    id: thisNameCard;
    // Spacing
    spacing: 10;
    // Anchors
    anchors.top: parent.top;
    anchors {
        topMargin: (parent.height - contentHeight)/2;
        bottomMargin: (parent.height - contentHeight)/2;
        leftMargin: 10;
        rightMargin: 10;
    }

    // Properties
    property int contentHeight: 50;
    property string displayName: "";
    property string userName: "";
    property int displayTextHeight: 18;
    property int usernameTextHeight: 12;

    Column {
        id: avatarImage;
        // Size
        height: contentHeight;
        width: height;
        Image {
            id: userImage;
            source: "../../icons/defaultNameCardUser.png"
            // Anchors
            width: parent.width;
            height: parent.height;
        }
    }
    Column {
        id: textContainer;
        // Size
        width: parent.width - avatarImage.width - parent.anchors.leftMargin - parent.anchors.rightMargin - parent.spacing;
        height: contentHeight;

        // DisplayName Text
        FiraSansSemiBold {
            id: displayNameText;
            // Properties
            text: thisNameCard.displayName;
            elide: Text.ElideRight;
            // Size
            width: parent.width;
            // Text Size
            size: thisNameCard.displayTextHeight;
            // Text Positioning
            verticalAlignment: Text.AlignVCenter;
        }

        // UserName Text
        FiraSansSemiBold {
            id: userNameText;
            // Properties
            text: thisNameCard.userName;
            elide: Text.ElideRight;
            visible: thisNameCard.displayName;
            // Size
            width: parent.width;
            // Text Size
            size: thisNameCard.usernameTextHeight;
            // Text Positioning
            verticalAlignment: Text.AlignVCenter;
        }

        // Spacer
        Item {
            height: 7;
            width: parent.width;
        }

        // VU Meter
        Rectangle {
            id: nameCardVUMeter;
            objectName: "AvatarInputs";
            width: parent.width;
            height: 4;
            // Avatar Audio VU Meter
            Item {
                id: controls;
                width: nameCardVUMeter.width;

                Rectangle {
                    anchors.fill: parent;
                    color: nameCardVUMeter.audioClipping ? "red" : "#696969";

                    Item {
                        id: audioMeter
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: nameCardVUMeter.iconPadding
                        anchors.right: parent.right
                        anchors.rightMargin: nameCardVUMeter.iconPadding
                        height: 8
                        Rectangle {
                            id: blueRect
                            color: "blue"
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            width: parent.width / 4
                        }
                        Rectangle {
                            id: greenRect
                            color: "green"
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.left: blueRect.right
                            anchors.right: redRect.left
                        }
                        Rectangle {
                            id: redRect
                            color: "red"
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            width: parent.width / 5
                        }
                        Rectangle {
                            z: 100
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            width: (1.0 - nameCardVUMeter.audioLevel) * parent.width
                            color: "#dddddd";
                        }
                    }
                }
            }
        }
    }
}
