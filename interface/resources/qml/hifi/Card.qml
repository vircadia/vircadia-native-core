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
import QtGraphicalEffects 1.0
import "../styles-uit"

Rectangle {
    property var goFunction: null;
    property alias image: lobby;
    property alias placeText: place.text;
    property alias usersText: users.text;
    property int textPadding: 20;
    property int textSize: 24;
    property string defaultThumbnail: Qt.resolvedUrl("../../images/default-domain.gif");
    property string thumbnail: defaultThumbnail;
    property string path: "";
    HifiConstants { id: hifi }
    Image {
        id: lobby;
        width: parent.width;
        height: parent.height;
        source: thumbnail || defaultThumbnail;
        fillMode: Image.PreserveAspectCrop;
        // source gets filled in later
        anchors.verticalCenter: parent.verticalCenter;
        anchors.left: parent.left;
        onStatusChanged: {
            if (status == Image.Error) {
                console.log("source: " + source + ": failed to load " + path);
                source = defaultThumbnail;
            }
        }
    }
    property int dropHorizontalOffset: 0;
    property int dropVerticalOffset: 1;
    property int dropRadius: 2;
    property int dropSamples: 9;
    property int dropSpread: 0;
    DropShadow {
        source: place;
        anchors.fill: place;
        horizontalOffset: dropHorizontalOffset;
        verticalOffset: dropVerticalOffset;
        radius: dropRadius;
        samples: dropSamples;
        color: hifi.colors.black;
        spread: dropSpread;
    }
    DropShadow {
        source: users;
        anchors.fill: users;
        horizontalOffset: dropHorizontalOffset;
        verticalOffset: dropVerticalOffset;
        radius: dropRadius;
        samples: dropSamples;
        color: hifi.colors.black;
        spread: dropSpread;
    }
    RalewaySemiBold {
        id: place;
        color: hifi.colors.white;
        size: textSize;
        anchors {
            top: parent.top;
            left: parent.left;
            margins: textPadding;
        }
    }
    RalewayRegular {
        id: users;
        size: textSize;
        color: hifi.colors.white;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            margins: textPadding;
        }
    }
    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction(parent);
        hoverEnabled: true;
    }
}
