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
    property string userName: "";
    property string placeName: "";
    property string action: "";
    property string timestamp: "";
    property string hifiUrl: "";
    property string thumbnail: defaultThumbnail;
    property var goFunction: null;
    property string storyId: "";
    property bool isConcurrency: action === 'concurrency';

    property string timePhrase: pastTime(timestamp);
    property int onlineUsers: 0;

    property int textPadding: 20;
    property int textSize: 24;
    property int textSizeSmall: 18;
    property string defaultThumbnail: Qt.resolvedUrl("../../images/default-domain.gif");
    HifiConstants { id: hifi }

    function pastTime(timestamp) { // Answer a descriptive string
        timestamp = new Date(timestamp);
        var then = timestamp.getTime(),
            now = Date.now(),
            since = now - then,
            ONE_MINUTE = 1000 * 60,
            ONE_HOUR = ONE_MINUTE * 60,
            hours = since / ONE_HOUR,
            minutes = (hours % 1) * 60;
        if (hours > 24) {
            return timestamp.toDateString();
        }
        if (hours > 1) {
            return Math.floor(hours).toString() + ' hr ' + Math.floor(minutes) + ' min ago';
        }
        if (minutes >= 2) {
            return Math.floor(minutes).toString() + ' min ago';
        }
        return 'about a minute ago';
    }

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
                console.log("source: " + source + ": failed to load " + hifiUrl);
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
        visible: desktop.gradientsSupported;
        source: place;
        anchors.fill: place;
        horizontalOffset: dropHorizontalOffset;
        verticalOffset: dropVerticalOffset;
        radius: dropRadius;
        samples: dropSamples;
        color: hifi.colors.black;
        spread: dropSpread;
    }
    property bool usersVisible: true;
    DropShadow {
        visible: users.visible && desktop.gradientsSupported;
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
        text: placeName;
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
        visible: usersVisible && isConcurrency;
        text: onlineUsers + ((onlineUsers === 1) ? ' person' : ' people');
        size: textSizeSmall;
        color: hifi.colors.white;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            margins: textPadding;
        }
    }
    Image {
        id: usersImage;
        source: "../../images/" + action + ".svg";
        width: 100;
        fillMode: Image.PreserveAspectFit;
        visible: usersVisible && !isConcurrency;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            margins: textPadding;
        }
    }
    Image {
        id: placeInfo;
        source: "../../images/more-info.svg"
        anchors.fill: place;
        fillMode: Image.PreserveAspectFit;
        visible: false;
    }
    Image {
        id: usersInfo;
        source: "../../images/more-info.svg"
        anchors.fill: users;
        fillMode: Image.PreserveAspectFit;
        visible: false;
    }
    // These two can be supplied to provide hover behavior.
    // For example, AddressBarDialog provides functions that set the current list view item
    // to that which is being hovered over.
    property var hoverThunk: function () { };
    property var unhoverThunk: function () { };
    MouseArea {
        id: zmouseArea;
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction("hifi://" + hifiUrl);
        hoverEnabled: true;
        onEntered: hoverThunk();
        onExited: unhoverThunk();
    }
    MouseArea {
        id: placeMouseArea;
        anchors.fill: place;
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction("/places/" + placeName);
        hoverEnabled: true;
        onEntered: { place.visible = false; placeInfo.visible = true; }
        onExited: { placeInfo.visible = false; place.visible = true; }
    }
    MouseArea {
        id: usersMouseArea;
        anchors.fill: users;
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction("/user_stories/" + storyId);
        hoverEnabled: true;
        onEntered: { usersVisible = false; usersInfo.visible = true; }
        onExited: { usersInfo.visible = false; usersVisible = true; }
    }
}
