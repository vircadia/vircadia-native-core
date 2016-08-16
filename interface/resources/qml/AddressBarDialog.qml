//
//  AddressBarDialog.qml
//
//  Created by Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import "controls"
import "styles"
import "windows"
import "hifi"
import "hifi/toolbars"

Window {
    id: root
    HifiConstants { id: hifi }

    objectName: "AddressBarDialog"
    frame: HiddenFrame {}
    hideBackground: true

    shown: false
    destroyOnHidden: false
    resizable: false
    scale: 1.25  // Make this dialog a little larger than normal

    width: addressBarDialog.implicitWidth
    height: addressBarDialog.implicitHeight

    onShownChanged: addressBarDialog.observeShownChanged(shown);
    Component.onCompleted: {
        root.parentChanged.connect(center);
        center();
    }
    Component.onDestruction: {
        root.parentChanged.disconnect(center);
    }

    function center() {
        // Explicitly center in order to avoid warnings at shutdown
        anchors.centerIn = parent;
    }

    function goCard(card) {
        if (useFeed) {
            storyCard.imageUrl = card.imageUrl;
            storyCard.userName = card.userName;
            storyCard.placeName = card.placeName;
            storyCard.actionPhrase = card.actionPhrase;
            storyCard.timePhrase = card.timePhrase;
            storyCard.hifiUrl = card.hifiUrl;
            storyCard.visible = true;
            return;
        }
        addressLine.text = card.hifiUrl;
        toggleOrGo(true);
    }
    property bool useFeed: false;
    property var allPlaces: [];
    property var allStories: [];
    property int cardWidth: 200;
    property int cardHeight: 152;
    property string metaverseBase: "https://metaverse.highfidelity.com/api/v1/";

    AddressBarDialog {
        id: addressBarDialog
        implicitWidth: backgroundImage.width
        implicitHeight: backgroundImage.height
        // The buttons have their button state changed on hover, so we have to manually fix them up here
        onBackEnabledChanged: backArrow.buttonState = addressBarDialog.backEnabled ? 1 : 0;
        onForwardEnabledChanged: forwardArrow.buttonState = addressBarDialog.forwardEnabled ? 1 : 0;

        ListModel { id: suggestions }

        ListView {
            id: scroll
            width: backgroundImage.width;
            height: cardHeight;
            spacing: hifi.layout.spacing;
            clip: true;
            anchors {
                bottom: backgroundImage.top;
                bottomMargin: 2 * hifi.layout.spacing;
                horizontalCenter: backgroundImage.horizontalCenter
            }
            model: suggestions;
            orientation: ListView.Horizontal;
            delegate: Card {
                width: cardWidth;
                height: cardHeight;
                goFunction: goCard;
                userName: model.username;
                placeName: model.place_name;
                hifiUrl: model.place_name + model.path;
                imageUrl: model.image_url;
                thumbnail: model.thumbnail_url;
                action: model.action;
                timestamp: model.created_at;
                onlineUsers: model.online_users;
                hoverThunk: function () { ListView.view.currentIndex = index; }
                unhoverThunk: function () { ListView.view.currentIndex = -1; }
            }
            highlightMoveDuration: -1;
            highlightMoveVelocity: -1;
            highlight: Rectangle { color: "transparent"; border.width: 4; border.color: "#1DB5ED"; z: 1; }
            leftMargin: 50; // Start the first item over be about the same amount as the last item peeks through on the other side.
            rightMargin: 50;
        }
        Image { // Just a visual indicator that the user can swipe the cards over to see more.
            source: "../images/Swipe-Icon-single.svg"
            width: 50;
            anchors {
                right: scroll.right;
                verticalCenter: scroll.verticalCenter;
            }
        }
        Image {
            id: backgroundImage
            source: "../images/address-bar.svg"
            width: 576 * root.scale
            height: 80 * root.scale
            property int inputAreaHeight: 56.0 * root.scale  // Height of the background's input area
            property int inputAreaStep: (height - inputAreaHeight) / 2

            ToolbarButton {
                id: homeButton
                imageURL: "../images/home.svg"
                buttonState: 1
                defaultState: 1
                hoverState: 2
                onClicked: addressBarDialog.loadHome();
                anchors {
                    left: parent.left
                    leftMargin: homeButton.width / 2
                    verticalCenter: parent.verticalCenter
                }
            }

            ToolbarButton {
                id: backArrow;
                imageURL: "../images/backward.svg";
                hoverState: addressBarDialog.backEnabled ? 2 : 0;
                defaultState: addressBarDialog.backEnabled ? 1 : 0;
                buttonState: addressBarDialog.backEnabled ? 1 : 0;
                onClicked: addressBarDialog.loadBack();
                anchors {
                    left: homeButton.right
                    verticalCenter: parent.verticalCenter
                }
            }
            ToolbarButton {
                id: forwardArrow;
                imageURL: "../images/forward.svg";
                hoverState: addressBarDialog.forwardEnabled ? 2 : 0;
                defaultState: addressBarDialog.forwardEnabled ? 1 : 0;
                buttonState: addressBarDialog.forwardEnabled ? 1 : 0;
                onClicked: addressBarDialog.loadForward();
                anchors {
                    left: backArrow.right
                    verticalCenter: parent.verticalCenter
                }
            }

            // FIXME replace with TextField
            TextInput {
                id: addressLine
                focus: true
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: forwardArrow.right
                    right: placesButton.left
                    leftMargin: forwardArrow.width
                    rightMargin: placesButton.width
                    topMargin: parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + hifi.layout.spacing
                }
                font.pixelSize: hifi.fonts.pixelSize * root.scale * 0.75
                helperText: "Go to: place, @user, /path, network address"
                helperPixelSize: font.pixelSize * 0.75
                helperItalic: true
                onTextChanged: filterChoicesByText()
            }
            // These two are radio buttons.
            ToolbarButton {
                id: placesButton
                imageURL: "../images/places.svg"
                buttonState: 1
                defaultState: useFeed ? 0 : 1;
                hoverState: useFeed ? 2 : -1;
                onClicked: useFeed ? toggleFeed() : identity()
                anchors {
                    right: feedButton.left;
                    bottom: addressLine.bottom;
                }
            }
            ToolbarButton {
                id: feedButton;
                imageURL: "../images/snap-feed.svg";
                buttonState: 0
                defaultState: useFeed ? 1 : 0;
                hoverState: useFeed ? -1 : 2;
                onClicked: useFeed ? identity() : toggleFeed();
                anchors {
                    right: parent.right;
                    bottom: addressLine.bottom;
                    rightMargin: feedButton.width / 2
                }
            }
        }

        UserStoryCard {
            id: storyCard;
            visible: false;
            visitPlace: function (hifiUrl) {
                storyCard.visible = false;
                addressLine.text = hifiUrl;
                toggleOrGo(true);
            };
            anchors {
                verticalCenter: scroll.verticalCenter;
                horizontalCenter: scroll.horizontalCenter;
                verticalCenterOffset: 50;
            }
        }
    }


    function toggleFeed () {
        useFeed = !useFeed;
        placesButton.buttonState = useFeed ? 0 : 1;
        feedButton.buttonState = useFeed ? 1 : 0;
        filterChoicesByText();
    }
    function getRequest(url, cb) { // cb(error, responseOfCorrectContentType) of url. General for 'get' text/html/json, but without redirects.
        // TODO: make available to other .qml.
        var request = new XMLHttpRequest();
        // QT bug: apparently doesn't handle onload. Workaround using readyState.
        request.onreadystatechange = function () {
            var READY_STATE_DONE = 4;
            var HTTP_OK = 200;
            if (request.readyState >= READY_STATE_DONE) {
                var error = (request.status !== HTTP_OK) && request.status.toString() + ':' + request.statusText,
                    response = !error && request.responseText,
                    contentType = !error && request.getResponseHeader('content-type');
                if (!error && contentType.indexOf('application/json') === 0) {
                    try {
                        response = JSON.parse(response);
                    } catch (e) {
                        error = e;
                    }
                }
                cb(error, response);
            }
        };
        request.open("GET", url, true);
        request.send();
    }
    function asyncMap(array, iterator, cb) {
        // call iterator(element, icb) once for each element of array, and then cb(error, mappedResult)
        // when icb(error, mappedElement) has been called by each iterator.
        // Calls to iterator are overlapped and map call icb in any order, but the mappedResults are collected in the same
        // order as the elements of the array.
        // short-circuits if error. Note that iterator MUST be an asynchronous function. (Use setTimeout if necessary.)
        var count = array.length, results = [];
        if (!count) {
            return cb(null, results);
        }
        array.forEach(function (element, index) {
            if (count < 0) { // don't keep iterating after we short-circuit
                return;
            }
            iterator(element, function (error, mapped) {
                results[index] = mapped;
                if (error || !--count) {
                    count = 1; // don't cb multiple times if error
                    cb(error, results);
                }
            });
        });
    }
    // Example:
    /*asyncMap([0, 1, 2, 3, 4, 5, 6], function (elt, icb) {
        console.log('called', elt);
        setTimeout(function () {
            console.log('answering', elt);
            icb(null, elt);
        }, Math.random() * 1000);
    }, console.log); */

    function identity(x) {
        return x;
    }

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

    function getPlace(placeData, cb) { // cb(error, side-effected-placeData), after adding path, thumbnails, and description
        var url = metaverseBase + 'places/' + placeData.place_name;
        getRequest(url, function (error, data) {
            if (handleError(url, error, data, cb)) {
                return;
            }
            var place = data.data.place, previews = place.previews;
            placeData.path = place.path;
            if (previews && previews.thumbnail) {
                placeData.thumbnail_url = previews.thumbnail;
            }
            if (place.description) {
                placeData.description = place.description;
                placeData.searchText += ' ' + place.description.toUpperCase();
            }
            cb(error, placeData);
        });
    }
    function makeModelData(data, optionalPlaceName) { // create a new obj from data
        // ListModel elements will only ever have those properties that are defined by the first obj that is added.
        // So here we make sure that we have all the properties we need, regardless of whether it is a place data or user story.
        var name = optionalPlaceName || data.place_name,
            tags = data.tags || [data.action, data.username],
            description = data.description || "",
            thumbnail_url = data.thumbnail_url || "",
            image_url = thumbnail_url;
        if (data.details) {
            try {
                image_url = JSON.parse(data.details).image_url || thumbnail_url;
            } catch (e) {
                console.log(name, "has bad details", data.details);
            }
        }
        return {
            place_name: name,
            username: data.username || "",
            path: data.path || "",
            created_at: data.created_at || "",
            action: data.action || "",
            thumbnail_url: thumbnail_url,
            image_url: image_url,

            tags: tags,
            description: description,
            online_users: data.online_users || 0,

            searchText: [name].concat(tags, description || []).join(' ').toUpperCase()
        }
    }
    function mapDomainPlaces(domain, cb) { // cb(error, arrayOfDomainPlaceData)
        function addPlace(name, icb) {
            getPlace(makeModelData(domain, name), icb);
        }
        // IWBNI we could get these results in order with most-recent-entered first.
        // In any case, we don't really need to preserve the domain.names order in the results.
        asyncMap(domain.names || [], addPlace, cb);
    }

    function suggestable(place) {
        if (useFeed) {
            return true;
        }
        return (place.place_name !== AddressManager.hostname) // Not our entry, but do show other entry points to current domain.
            && place.thumbnail_url
            && place.online_users // at least one present means it's actually online
            && place.online_users <= 20;
    }
    function getDomainPage(pageNumber, cb) { // cb(error) after all pages of domain data have been added to model
        // Each page of results is processed completely before we start on the next page.
        // For each page of domains, we process each domain in parallel, and for each domain, process each place name in parallel.
        // This gives us minimum latency within the page, but we do preserve the order within the page by using asyncMap and
        // only appending the collected results.
        var params = [
            'open', // published hours handle now
            // FIXME: should determine if place is actually running
            'restriction=open', // Not by whitelist, etc.  FIXME: If logged in, add hifi to the restriction options, in order to include places that require login.
            // FIXME add maturity
            'protocol=' + encodeURIComponent(AddressManager.protocolVersion()),
            'sort_by=users',
            'sort_order=desc',
            'page=' + pageNumber
        ];
        var url = metaverseBase + 'domains/all?' + params.join('&');
        getRequest(url, function (error, data) {
            if (handleError(url, error, data, cb)) {
                return;
            }
            asyncMap(data.data.domains, mapDomainPlaces, function (error, pageResults) {
                if (error) {
                    return cb(error);
                }
                // pageResults is now [ [ placeDataOneForDomainOne, placeDataTwoForDomainOne, ...], [ placeDataTwoForDomainTwo...] ]
                pageResults.forEach(function (domainResults) {
                    allPlaces = allPlaces.concat(domainResults);
                    if (!addressLine.text && !useFeed) { // Don't add if the user is already filtering
                        domainResults.forEach(function (place) {
                            if (suggestable(place)) {
                                suggestions.append(place);
                            }
                        });
                    }
                });
                if (data.current_page < data.total_pages) {
                    return getDomainPage(pageNumber + 1, cb);
                }
                cb();
            });
        });
    }
    function getUserStoryPage(pageNumber, cb) { // cb(error) after all pages of domain data have been added to model
        var url = metaverseBase + 'user_stories?page=' + pageNumber;
        getRequest(url, function (error, data) {
            if (handleError(url, error, data, cb)) {
                return;
            }
            var stories = data.user_stories.map(function (story) { // explicit single-argument function
                return makeModelData(story);
            });
            allStories = allStories.concat(stories);
            if (!addressLine.text && useFeed) { // Don't add if the user is already filtering
                stories.forEach(function (story) {
                    suggestions.append(story);
                });
            }
            if ((data.current_page < data.total_pages) && (data.current_page <=  10)) { // just 10 pages = 100 stories for now
                return getUserStoryPage(pageNumber + 1, cb);
            }
            cb();
        });
    }
    function filterChoicesByText() {
        suggestions.clear();
        var words = addressLine.text.toUpperCase().split(/\s+/).filter(identity),
            data = useFeed ? allStories : allPlaces;
        function matches(place) {
            if (!words.length) {
                return suggestable(place);
            }
            return words.every(function (word) {
                return place.searchText.indexOf(word) >= 0;
            });
        }
        data.forEach(function (place) {
            if (matches(place)) {
                suggestions.append(place);
            }
        });
    }

    function fillDestinations() {
        allPlaces = [];
        allStories = [];
        suggestions.clear();
        getDomainPage(1, function (error) {
            console.log('domain query', error || 'ok', allPlaces.length);
        });
        getUserStoryPage(1, function (error) {
            console.log('user stories query', error || 'ok', allStories.length);
        });
    }

    onVisibleChanged: {
        if (visible) {
            addressLine.forceActiveFocus()
            fillDestinations();
        } else {
            addressLine.text = ""
        }
    }

    function toggleOrGo(fromSuggestions) {
        if (addressLine.text !== "") {
            addressBarDialog.loadAddress(addressLine.text, fromSuggestions)
        }
        root.shown = false;
    }

    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                root.shown = false
                event.accepted = true
                break
            case Qt.Key_Enter:
            case Qt.Key_Return:
                toggleOrGo()
                event.accepted = true
                break
        }
    }
}
