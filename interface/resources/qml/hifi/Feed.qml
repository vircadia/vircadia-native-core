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
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.

Column {
    id: root;
    visible: !!suggestions.count;

    property int cardWidth: 212;
    property int cardHeight: 152;
    property int textPadding: 10;
    property int smallMargin: 4;
    property int messageHeight: 40;
    property int textSize: 24;
    property int textSizeSmall: 18;
    property int stackShadowNarrowing: 5;
    property int stackedCardShadowHeight: 4;
    property int labelSize: 20;

    property string protocol: '';
    property string actions: 'snapshot';
    // sendToScript doesn't get wired until after everything gets created. So we have to queue fillDestinations on nextTick.
    property string labelText: actions;
    property string filter: '';
    property var goFunction: null;
    property var http: null;

    property bool autoScrollTimerEnabled: false;

    HifiConstants { id: hifi }
    Component.onCompleted: suggestions.getFirstPage();
    HifiModels.PSFListModel {
        id: suggestions;
        http: root.http;
        property var options: [
            'include_actions=' + actions,
            'restriction=' + (Account.isLoggedIn() ? 'open,hifi' : 'open'),
            'require_online=true',
            'protocol=' + encodeURIComponent(Window.protocolSignature())
        ];
        endpoint: '/api/v1/user_stories?' + options.join('&');
        itemsPerPage: 4;
        processPage: function (data) {
            return data.user_stories.map(makeModelData);
        };
        listModelName: actions;
        listView: scroll;
        searchFilter: filter;
    }

    function resolveUrl(url) {
        return (url.indexOf('/') === 0) ? (Account.metaverseServerURL + url) : url;
    }
    function makeModelData(data) { // create a new obj from data
        // ListModel elements will only ever have those properties that are defined by the first obj that is added.
        // So here we make sure that we have all the properties we need, regardless of whether it is a place data or user story.
        var name = data.place_name,
            tags = data.tags || [data.action, data.username],
            description = data.description || "",
            thumbnail_url = data.thumbnail_url || "";
        return {
            place_name: name,
            username: data.username || "",
            path: data.path || "",
            created_at: data.created_at || data.updated_at || "",  // FIXME why aren't we getting created_at?
            action: data.action || "",
            thumbnail_url: resolveUrl(thumbnail_url),
            image_url: resolveUrl(data.details && data.details.image_url),

            metaverseId: (data.id || "").toString(), // Some are strings from server while others are numbers. Model objects require uniformity.

            tags: tags,
            description: description,
            online_users: data.details.connections || data.details.concurrency || 0,
            // Server currently doesn't give isStacked (undefined). Could give bool.
            drillDownToPlace: data.is_stacked || (data.action === 'concurrency'),
            isStacked: !!data.is_stacked,

            time_before_autoscroll_ms: data.hold_time || 3000
        };
    }

    RalewayBold {
        id: label;
        text: labelText;
        color: hifi.colors.blueAccent;
        size: labelSize;
    }
    ListView {
        id: scroll;
        model: suggestions;
        orientation: ListView.Horizontal;
        highlightFollowsCurrentItem: true;
        preferredHighlightBegin: 0;
        preferredHighlightEnd: cardWidth;
        highlightRangeMode: ListView.StrictlyEnforceRange;
        highlightMoveDuration: 800;
        highlightMoveVelocity: 1;
        currentIndex: -1;

        spacing: 12;
        width: parent.width;
        height: cardHeight + stackedCardShadowHeight;
        delegate: Card {
            id: card;
            width: cardWidth;
            height: cardHeight;
            goFunction: root.goFunction;
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
            isStacked: model.isStacked;

            textPadding: root.textPadding;
            smallMargin: root.smallMargin;
            messageHeight: root.messageHeight;
            textSize: root.textSize;
            textSizeSmall: root.textSizeSmall;
            stackShadowNarrowing: root.stackShadowNarrowing;
            shadowHeight: root.stackedCardShadowHeight;
            hoverThunk: function () { 
                hovered = true;
                if(root.autoScrollTimerEnabled) {
                    autoScrollTimer.stop();
                }
			}
            unhoverThunk: function () { 
                hovered = false;
                if(root.autoScrollTimerEnabled) {
                    autoScrollTimer.start();
                }
            }
        }

        onCountChanged: {
            if (scroll.currentIndex === -1 && scroll.count > 0 && root.autoScrollTimerEnabled) {
                scroll.currentIndex = 0;
                autoScrollTimer.interval = suggestions.get(scroll.currentIndex).time_before_autoscroll_ms;
                autoScrollTimer.start();
            }
        }

        onCurrentIndexChanged: {
            if (root.autoScrollTimerEnabled) {
                autoScrollTimer.interval = suggestions.get(scroll.currentIndex).time_before_autoscroll_ms;
                autoScrollTimer.start();
            }
        }
    }

    Timer {
        id: autoScrollTimer;
        interval: 3000;
        running: false;
        repeat: false;
        onTriggered: {
            if (scroll.currentIndex !== -1) {
                if (scroll.currentIndex === scroll.count - 1) {
                    scroll.currentIndex = 0;
                } else {
                    scroll.currentIndex++;
                }
            }
        }
    }
}
