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
import "toolbars"
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

    property bool drillDownToPlace: false;
    property bool showPlace: isConcurrency;
    property string messageColor: "#1DB5ED";
    property string timePhrase: pastTime(timestamp);
    property int onlineUsers: 0;
    property bool isConcurrency: action === 'concurrency';
    property bool isStacked: !isConcurrency && drillDownToPlace;

    property int textPadding: 10;
    property int messageHeight: 40;
    property int textSize: 24;
    property int textSizeSmall: 18;
    property string defaultThumbnail: Qt.resolvedUrl("../../images/default-domain.gif");
    property int shadowHeight: 20;
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
        width: parent.width - (isConcurrency ? 0 : 8);
        height: parent.height - messageHeight - (isConcurrency ? 0 : 4);
        source: thumbnail || defaultThumbnail;
        fillMode: Image.PreserveAspectCrop;
        // source gets filled in later
        anchors {
            horizontalCenter: parent.horizontalCenter;
            top: parent.top;
            topMargin: isConcurrency ? 0 : 4;
        }
        onStatusChanged: {
            if (status == Image.Error) {
                console.log("source: " + source + ": failed to load " + hifiUrl);
                source = defaultThumbnail;
            }
        }
    }
    Rectangle {
        id: shadow1;
        visible: isStacked;
        width: parent.width - 5;
        height: shadowHeight / 2;
        anchors {
            top: parent.bottom;
            horizontalCenter: parent.horizontalCenter;
        }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "gray" }
            GradientStop { position: 1.0; color: "white" }
        }
    }
    Rectangle {
        id: shadow2;
        visible: isStacked;
        width: shadow1.width - 5;
        height: shadowHeight / 2;
        anchors {
            top: shadow1.bottom;
            horizontalCenter: parent.horizontalCenter;
        }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "gray" }
            GradientStop { position: 1.0; color: "white" }
        }
    }
    property int dropHorizontalOffset: 0;
    property int dropVerticalOffset: 1;
    property int dropRadius: 2;
    property int dropSamples: 9;
    property int dropSpread: 0;
    DropShadow {
        visible: showPlace && desktop.gradientsSupported;
        source: place;
        anchors.fill: place;
        horizontalOffset: dropHorizontalOffset;
        verticalOffset: dropVerticalOffset;
        radius: dropRadius;
        samples: dropSamples;
        color: hifi.colors.black;
        spread: dropSpread;
    }
    RalewaySemiBold {
        id: place;
        visible: showPlace;
        text: placeName;
        color: hifi.colors.white;
        size: textSize;
        anchors {
            top: parent.top;
            left: parent.left;
            margins: textPadding;
        }
    }
    Row {
        FiraSansRegular {
            id: users;
            visible: isConcurrency;
            text: onlineUsers;
            size: textSize;
            color: messageColor;
            anchors.verticalCenter: message.verticalCenter;
        }
        RalewaySemiBold {
            id: message;
            text: isConcurrency ? ((onlineUsers === 1) ? "person" : "people") : (drillDownToPlace ? "snapshots" : ("by " + userName));
            size: textSizeSmall;
            color: messageColor;
            anchors {
                bottom: parent.bottom;
                bottomMargin: parent.spacing;
            }
        }
        spacing: textPadding;
        height: messageHeight;
        anchors {
            bottom: parent.bottom;
            left: parent.left;
            leftMargin: textPadding;
        }
    }
    // These two can be supplied to provide hover behavior.
    // For example, AddressBarDialog provides functions that set the current list view item
    // to that which is being hovered over.
    property var hoverThunk: function () { };
    property var unhoverThunk: function () { };
    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction("hifi://" + hifiUrl);
        hoverEnabled: true;
        onEntered: hoverThunk();
        onExited: unhoverThunk();
    }
    StateImage {
        id: actionIcon;
        imageURL: "../../images/" + action + ".svg";
        size: 32;
        buttonState: 1;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            margins: 4;
        }
    }
    MouseArea {
        width: parent.width;
        height: messageHeight;
        anchors {
            top: lobby.bottom;
        }
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction(drillDownToPlace ? ("/places/" + placeName) : ("/user_stories/" + storyId));
        hoverEnabled: true;
        onEntered: actionIcon.buttonState = 0;
        onExited: actionIcon.buttonState = 1;
    }
}
