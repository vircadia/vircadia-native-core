//
//  UserStoryCard.qml
//  qml/hifi
//
//  Displays a clickable card representing a user story or destination.
//
//  Created by Howard Stearns on 8/11/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import "../styles-uit" as HifiStyles
import "../controls-uit" as HifiControls
            
Rectangle {
    id: storyCard;
    width: 500;
    height: 330;
    property string userName: "User";
    property string placeName: "Home";
    property string actionPhrase: "did something";
    property string timePhrase: "";
    property string hifiUrl: storyCard.placeName;
    property string imageUrl: Qt.resolvedUrl("../../images/default-domain.gif");
    property var visitPlace: function (ignore) { };
    color: "white";
    HifiStyles.HifiConstants { id: otherHifi }
    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: storyCard.visible = false;
        hoverEnabled: true;
        // The content of the storyCard has buttons. For these to work without being
        // blanketed by the MouseArea, they need to be children of the MouseArea.
        Image {
            id: storyImage;
            source: storyCard.imageUrl;
            width: storyCard.width - 100;
            height: storyImage.width / 1.91;
            fillMode: Image.PreserveAspectCrop;
            anchors {
                horizontalCenter: parent.horizontalCenter;
                top: parent.top;
                topMargin: 20;
            }
        }
        HifiStyles.RalewayRegular {
            id: storyLabel;
            text: storyCard.userName + " " + storyCard.actionPhrase + " in " + storyCard.placeName
            size: 20;
            color: "black"
            anchors {
                horizontalCenter: storyImage.horizontalCenter;
                top: storyImage.bottom;
                topMargin: hifi.layout.spacing
            }
        }
        HifiStyles.RalewayRegular {
            text: storyCard.timePhrase;
            size: 15;
            color: "slategrey"
            anchors {
                verticalCenter: visitButton.verticalCenter;
                left: storyImage.left;
            }
        }
        HifiControls.Button {
            id: visitButton;
            text: "visit " + storyCard.placeName;
            color: otherHifi.buttons.blue;
            onClicked: visitPlace(storyCard.hifiUrl);
            anchors {
                top: storyLabel.bottom;
                topMargin: hifi.layout.spacing;
                right: storyImage.right;
            }
        }                
    }
}
