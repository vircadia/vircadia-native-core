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
import TabletScriptingInterface 1.0

import "toolbars"
import "../styles-uit"

Item {
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
    property string messageColor: isAnnouncement ? "white" : hifi.colors.blueAccent;
    property string timePhrase: pastTime(timestamp);
    property int onlineUsers: 0;
    property bool isConcurrency: action === 'concurrency';
    property bool isAnnouncement: action === 'announcement';
    property bool isStacked: !isConcurrency && drillDownToPlace;

    property int textPadding: 10;
    property int smallMargin: 4;
    property int messageHeight: 40;
    property int textSize: 24;
    property int textSizeSmall: 18;
    property int stackShadowNarrowing: 5;
    property string defaultThumbnail: Qt.resolvedUrl("../../images/default-domain.gif");
    property int shadowHeight: 10;
    property bool hovered: false

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

    function pluralize(count, singular, optionalPlural) {
        return (count === 1) ? singular : (optionalPlural || (singular + "s"));
    }

    DropShadow {
        visible: isStacked;
        anchors.fill: shadow1;
        source: shadow1;
        verticalOffset: 2;
        radius: 4;
        samples: 9;
        color: hifi.colors.baseGrayShadow;
    }
    Rectangle {
        id: shadow1;
        visible: isStacked;
        width: parent.width - stackShadowNarrowing;
        height: shadowHeight;
        anchors {
            top: parent.bottom;
            horizontalCenter: parent.horizontalCenter;
        }
    }
    DropShadow {
        anchors.fill: base;
        source: base;
        verticalOffset: 2;
        radius: 4;
        samples: 9;
        color: hifi.colors.baseGrayShadow;
    }
    Rectangle {
        id: base;
        color: "white";
        anchors.fill: parent;
    }

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
        height: parent.height -(isAnnouncement ? smallMargin : messageHeight) - (isConcurrency ? 0 : smallMargin);
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
    property int dropHorizontalOffset: 0;
    property int dropVerticalOffset: 1;
    property int dropRadius: 2;
    property int dropSamples: 9;
    property int dropSpread: 0;
    DropShadow {
        visible: showPlace; // Do we have to check for whatever the modern equivalent is for desktop.gradientsSupported?
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
    Rectangle {
        id: lozenge;
        visible: isAnnouncement;
        color: lozengeHot.containsMouse ? hifi.colors.redAccent : hifi.colors.redHighlight;
        anchors.fill: infoRow;
        radius: lozenge.height / 2.0;
    }
    Row {
        id: infoRow;
        Image {
            id: icon;
            source: isAnnouncement ? "../../images/Announce-Blast.svg" : "../../images/snap-icon.svg";
            width: 40;
            height: 40;
            visible: ((action === 'snapshot') || isAnnouncement) && (messageHeight >= 40);
        }
        FiraSansRegular {
            id: users;
            visible: isConcurrency || isAnnouncement;
            text: onlineUsers;
            size: textSize;
            color: messageColor;
            anchors.verticalCenter: message.verticalCenter;
        }
        RalewayRegular {
            id: message;
            visible: !isAnnouncement;
            text: isConcurrency ? pluralize(onlineUsers, "person", "people") : (drillDownToPlace ? "snapshots" : ("by " + userName));
            size: textSizeSmall;
            color: messageColor;
            elide: Text.ElideRight; // requires a width to be specified`
            width: root.width - textPadding
                - (icon.visible ? icon.width + parent.spacing : 0)
                - (users.visible ? users.width + parent.spacing : 0)
                - (actionIcon.width + (2 * smallMargin));
            anchors {
                bottom: parent.bottom;
                bottomMargin: parent.spacing;
            }
        }
        Column {
            visible: isAnnouncement;
            RalewayRegular {
                text: pluralize(onlineUsers, "connection") + "   "; // hack padding
                size: textSizeSmall;
                color: messageColor;
            }
            RalewayRegular {
                text: pluralize(onlineUsers, "is here now", "are here now");
                size: textSizeSmall * 0.7;
                color: messageColor;
            }
        }
        spacing: textPadding;
        height: messageHeight;
        anchors {
            bottom: parent.bottom;
            left: parent.left;
            leftMargin: textPadding;
            bottomMargin: isAnnouncement ? textPadding : 0;
        }
    }
    // These two can be supplied to provide hover behavior.
    // For example, AddressBarDialog provides functions that set the current list view item
    // to that which is being hovered over.
    property var hoverThunk: function () { };
    property var unhoverThunk: function () { };
    Rectangle {
        anchors.fill: parent;
        visible: root.hovered
        color: "transparent";
        border.width: 4; border.color: hifiStyleConstants.colors.primaryHighlight;
        z: 1;
    }
    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: {
            Tablet.playSound(TabletEnums.ButtonClick);
            goFunction("hifi://" + hifiUrl);
        }
        hoverEnabled: true;
        onEntered:  {
            Tablet.playSound(TabletEnums.ButtonHover);
            hoverThunk();
        }
        onExited: unhoverThunk();
    }
    StateImage {
        id: actionIcon;
        visible: !isAnnouncement;
        imageURL: "../../images/info-icon-2-state.svg";
        size: 30;
        buttonState: messageArea.containsMouse ? 1 : 0;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            margins: smallMargin;
        }
    }
    function go() {
        Tablet.playSound(TabletEnums.ButtonClick);
        goFunction(drillDownToPlace ? ("/places/" + placeName) : ("/user_stories/" + storyId));
    }
    MouseArea {
        id: messageArea;
        visible: !isAnnouncement;
        width: parent.width;
        height: messageHeight;
        anchors.top: lobby.bottom;
        acceptedButtons: Qt.LeftButton;
        onClicked: go();
        hoverEnabled: true;
    }
    MouseArea {
        id: lozengeHot;
        visible: lozenge.visible;
        anchors.fill: lozenge;
        acceptedButtons: Qt.LeftButton;
        onClicked: go();
        hoverEnabled: true;
    }
}
