//
//  Card.qml
//  qml/hifi
//
//  Displays a clickable card representing a user story or destination.
//
//  Created by Howard Stearns on 7/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import "../styles-uit"

Rectangle {
    property var goFunction: null;
    property var userStory: null;
    property alias image: lobby;
    property alias placeText: place.text;
    property alias usersText: users.text;
    // FIXME: let's get our own
    property string defaultPicture: "http://www.davidluke.com/wp-content/themes/david-luke/media/ims/placeholder720.gif";
    HifiConstants { id: hifi }
    Image {
        id: lobby;
        width: parent.width;
        height: parent.height;
        source: defaultPicture;
        fillMode: Image.PreserveAspectCrop;
        // source gets filled in later
        anchors.verticalCenter: parent.verticalCenter;
        anchors.left: parent.left;
        onStatusChanged: {
            if (status == Image.Error) {
                console.log("source: " + source + ": failed to load " + JSON.stringify(userStory));
                source = defaultPicture;
            }
        }
    }
    RalewaySemiBold {
        id: place;
        color: hifi.colors.lightGrayText;
        anchors {
            leftMargin: 2 * hifi.layout.spacing;
            topMargin: 2 * hifi.layout.spacing;
        }
    }
    RalewayRegular {
        id: users;
        color: hifi.colors.lightGrayText;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            bottomMargin: 2 * hifi.layout.spacing;
            rightMargin: 2 * hifi.layout.spacing;
        }
    }
    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction(parent);
    }
}
