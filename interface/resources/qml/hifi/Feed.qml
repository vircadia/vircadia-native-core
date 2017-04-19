//
//  Feed.qml
//  qml/hifi
//
//  Displays a particular type of feed
//
//  Created by Howard Stearns on 4/18/2017
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

    property int cardWidth: 212;
    property int cardHeight: 152;
    property int stackedCardShadowHeight: 10;
    property alias suggestions: feed;

    HifiConstants { id: hifi }
    ListModel { id: feed; }
    
    RalewayLight {
        id: label;
        text: "Places";
        color: hifi.colors.blueHighlight;
        size: 38;
        width: root.width;
        anchors {
            top: parent.top;
            left: parent.left;
            margins: 10;
        }
    }

    ListView {
        id: scroll;
        clip: true;
        model: feed;
        orientation: ListView.Horizontal;

        spacing: 14;
        height: cardHeight + stackedCardShadowHeight;
        anchors {
            top: label.bottom;
            left: parent.left;
            right: parent.right;
            leftMargin: 10;
        }

        delegate: Card {
            width: cardWidth;
            height: cardHeight;
            goFunction: goCard;
            userName: model.username;
            placeName: model.place_name;
            hifiUrl: model.place_name + model.path;
            thumbnail: model.thumbnail_url;
            imageUrl: model.image_url;
            action: model.action;
            timestamp: model.created_at;
            onlineUsers: model.online_users;
            storyId: model.metaverseId;
            drillDownToPlace: model.drillDownToPlace;
            shadowHeight: stackedCardShadowHeight;
            hoverThunk: function () { scroll.currentIndex = index; }
            unhoverThunk: function () { scroll.currentIndex = -1; }
        }
    }
}
