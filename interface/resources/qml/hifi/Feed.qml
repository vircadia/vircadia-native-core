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

Column {
    id: root;
    visible: false;

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

    property string metaverseServerUrl: '';
    property string actions: 'snapshot';
    // sendToScript doesn't get wired until after everything gets created. So we have to queue fillDestinations on nextTick.
    property string labelText: actions;
    property string filter: '';
    onFilterChanged: filterChoicesByText();
    property var goFunction: null;
    property var rpc: null;

    HifiConstants { id: hifi }
    ListModel { id: suggestions; }

    function resolveUrl(url) {
        return (url.indexOf('/') === 0) ? (metaverseServerUrl + url) : url;
    }
    function makeModelData(data) { // create a new obj from data
        // ListModel elements will only ever have those properties that are defined by the first obj that is added.
        // So here we make sure that we have all the properties we need, regardless of whether it is a place data or user story.
        var name = data.place_name,
            tags = data.tags || [data.action, data.username],
            description = data.description || "",
            thumbnail_url = data.thumbnail_url || "";
        if (actions === 'concurrency,snapshot') {
            // A temporary hack for simulating announcements. We won't use this in production, but if requested, we'll use this data like announcements.
            data.details.connections = 4;
            data.action = 'announcement';
        }
        return {
            place_name: name,
            username: data.username || "",
            path: data.path || "",
            created_at: data.created_at || "",
            action: data.action || "",
            thumbnail_url: resolveUrl(thumbnail_url),
            image_url: resolveUrl(data.details && data.details.image_url),

            metaverseId: (data.id || "").toString(), // Some are strings from server while others are numbers. Model objects require uniformity.

            tags: tags,
            description: description,
            online_users: data.details.connections || data.details.concurrency || 0,
            drillDownToPlace: false,

            searchText: [name].concat(tags, description || []).join(' ').toUpperCase()
        }
    }
    property var allStories: [];
    property var placeMap: ({}); // Used for making stacks.
    property int requestId: 0;
    function handleError(url, error, data, cb) { // cb(error) and answer truthy if needed, else falsey
        if (!error && (data.status === 'success')) {
            return;
        }
        if (!error) { // Create a message from the data
            error = data.status + ': ' + data.error;
        }
        if (typeof(error) === 'string') { // Make a proper Error object
            error = new Error(error);
        }
        error.message += ' in ' + url; // Include the url.
        cb(error);
        return true;
    }
    function getUserStoryPage(pageNumber, cb, cb1) { // cb(error) after all pages of domain data have been added to model
        // If supplied, cb1 will be run after the first page IFF it is not the last, for responsiveness.
        var options = [
            'now=' + new Date().toISOString(),
            'include_actions=' + actions,
            'restriction=' + (Account.isLoggedIn() ? 'open,hifi' : 'open'),
            'require_online=true',
            'protocol=' + encodeURIComponent(AddressManager.protocolVersion()),
            'page=' + pageNumber
        ];
        var url = metaverseBase + 'user_stories?' + options.join('&');
        var thisRequestId = ++requestId;
        rpc('request', url, function (error, data) {
            if (thisRequestId !== requestId) {
                error = 'stale';
            }
            if (handleError(url, error, data, cb)) {
                return; // abandon stale requests
            }
            allStories = allStories.concat(data.user_stories.map(makeModelData));
            if ((data.current_page < data.total_pages) && (data.current_page <=  10)) { // just 10 pages = 100 stories for now
                if ((pageNumber === 1) && cb1) {
                    cb1();
                }
                return getUserStoryPage(pageNumber + 1, cb);
            }
            cb();
        });
    }
    function fillDestinations() { // Public
        console.debug('Feed::fillDestinations()')

        function report(label, error) {
            console.log(label, actions, error || 'ok', allStories.length, 'filtered to', suggestions.count);
        }
        var filter = makeFilteredStoryProcessor(), counter = 0;
        allStories = [];
        suggestions.clear();
        placeMap = {};
        getUserStoryPage(1, function (error) {
            allStories.slice(counter).forEach(filter);
            report('user stories update', error);
            root.visible = !!suggestions.count;
        }, function () { // If there's more than a page, put what we have in the model right away, keeping track of how many are processed.
            allStories.forEach(function (story) {
                counter++;
                filter(story);
                root.visible = !!suggestions.count;
            });
            report('user stories');
        });
    }
    function identity(x) {
        return x;
    }
    function makeFilteredStoryProcessor() { // answer a function(storyData) that adds it to suggestions if it matches
        var words = filter.toUpperCase().split(/\s+/).filter(identity);
        function suggestable(story) {
            // We could filter out places we don't want to suggest, such as those where (story.place_name === AddressManager.placename) or (story.username === Account.username).
            return true;
        }
        function matches(story) {
            if (!words.length) {
                return suggestable(story);
            }
            return words.every(function (word) {
                return story.searchText.indexOf(word) >= 0;
            });
        }
        function addToSuggestions(place) {
            var collapse = ((actions === 'concurrency,snapshot') && (place.action !== 'concurrency')) || (place.action === 'announcement');
            if (collapse) {
                var existing = placeMap[place.place_name];
                if (existing) {
                    existing.drillDownToPlace = true;
                    return;
                }
            }
            suggestions.append(place);
            if (collapse) {
                placeMap[place.place_name] = suggestions.get(suggestions.count - 1);
            } else if (place.action === 'concurrency') {
                suggestions.get(suggestions.count - 1).drillDownToPlace = true; // Don't change raw place object (in allStories).
            }
        }
        return function (story) {
            if (matches(story)) {
                addToSuggestions(story);
            }
        };
    }
    function filterChoicesByText() {
        suggestions.clear();
        placeMap = {};
        allStories.forEach(makeFilteredStoryProcessor());
        root.visible = !!suggestions.count;
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
        highlightFollowsCurrentItem: false
        highlightMoveDuration: -1;
        highlightMoveVelocity: -1;
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

            textPadding: root.textPadding;
            smallMargin: root.smallMargin;
            messageHeight: root.messageHeight;
            textSize: root.textSize;
            textSizeSmall: root.textSizeSmall;
            stackShadowNarrowing: root.stackShadowNarrowing;
            shadowHeight: root.stackedCardShadowHeight;
            hoverThunk: function () { hovered = true }
            unhoverThunk: function () { hovered = false }
        }
    }
    NumberAnimation {
        id: anim;
        target: scroll;
        property: "contentX";
        duration: 250;
    }
    function scrollToIndex(index) {
        anim.running = false;
        var pos = scroll.contentX;
        var destPos;
        scroll.positionViewAtIndex(index, ListView.Contain);
        destPos = scroll.contentX;
        anim.from = pos;
        anim.to = destPos;
        scroll.currentIndex = index;
        anim.running = true;
    }
}
