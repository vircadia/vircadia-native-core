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
    id: root;
    property string userName: "";
    property string placeName: "";
    property string action: "";
    property string timestamp: "";
    property string hifiUrl: "";
    property string thumbnail: defaultThumbnail;
    property string imageUrl: "";
    property var goFunction: null;
    property string storyId: "";

    property bool drillDownToPlace: false;
    property bool showPlace: isConcurrency;
    property string messageColor: hifi.colors.blueAccent;
    property string timePhrase: pastTime(timestamp);
    property int onlineUsers: 0;
    property bool isConcurrency: action === 'concurrency';
    property bool isStacked: !isConcurrency && drillDownToPlace;

    property int textPadding: 10;
    property int smallMargin: 4;
    property int messageHeight: 40;
    property int textSize: 24;
    property int textSizeSmall: 18;
    property int stackShadowNarrowing: 5;
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

    property bool hasGif: imageUrl.indexOf('.gif') === (imageUrl.length - 4);
    AnimatedImage {
        id: animation;
        // Always visible, to drive loading, but initially covered up by lobby during load.
        source: hasGif ? imageUrl : "";
        fillMode: lobby.fillMode;
        anchors.fill: lobby;
    }
    Image {
        id: lobby;
        visible: !hasGif || (animation.status !== Image.Ready);
        width: parent.width - (isConcurrency ? 0 : (2 * smallMargin));
        height: parent.height - messageHeight - (isConcurrency ? 0 : smallMargin);
        source: thumbnail || defaultThumbnail;
        fillMode: Image.PreserveAspectCrop;
        anchors {
            horizontalCenter: parent.horizontalCenter;
            top: parent.top;
            topMargin: isConcurrency ? 0 : smallMargin;
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
        width: parent.width - stackShadowNarrowing;
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
        width: shadow1.width - stackShadowNarrowing;
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
        elide: Text.ElideRight; // requires constrained width
        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
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
        Image {
            id: icon;
            source: "../../images/snap-icon.svg"
            width: 40;
            height: 40;
            visible: action === 'snapshot';
        }
        RalewayRegular {
            id: message;
            text: isConcurrency ? ((onlineUsers === 1) ? "person" : "people") : (drillDownToPlace ? "snapshots" : ("by " + userName));
            size: textSizeSmall;
            color: messageColor;
            elide: Text.ElideRight; // requires a width to be specified`
            width: root.width - textPadding
                - (users.visible ? users.width + parent.spacing : 0)
                - (icon.visible ? icon.width + parent.spacing : 0)
                - (actionIcon.width + (2 * smallMargin));
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
        imageURL: "../../images/info-icon-2-state.svg";
        size: 32;
        buttonState: messageArea.containsMouse ? 1 : 0;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            margins: smallMargin;
        }
    }
    MouseArea {
        id: messageArea;
        width: parent.width;
        height: messageHeight;
        anchors {
            top: lobby.bottom;
        }
        acceptedButtons: Qt.LeftButton;
        onClicked: goFunction(drillDownToPlace ? ("/places/" + placeName) : ("/user_stories/" + storyId));
        hoverEnabled: true;
    }
}
