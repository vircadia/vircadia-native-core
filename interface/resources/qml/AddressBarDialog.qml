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
import "styles-uit" as HifiStyles
import "controls-uit" as HifiControls

Window {
    id: root
    HifiConstants { id: hifi }
    HifiStyles.HifiConstants { id: hifiStyleConstants }

    objectName: "AddressBarDialog"
    title: "Go To:"

    shown: false
    destroyOnHidden: false
    resizable: false
    pinnable: false;

    width: addressBarDialog.implicitWidth
    height: addressBarDialog.implicitHeight
    property int gap: 14

    onShownChanged: {
        addressBarDialog.keyboardEnabled = HMD.active;
        addressBarDialog.observeShownChanged(shown);
    }
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

    function resetAfterTeleport() {
        storyCardFrame.shown = root.shown = false;
    }
    function goCard(targetString) {
        if (0 !== targetString.indexOf('hifi://')) {
            storyCardHTML.url = addressBarDialog.metaverseServerUrl + targetString;
            storyCardFrame.shown = true;
            return;
        }
        addressLine.text = targetString;
        toggleOrGo(true);
        clearAddressLineTimer.start();
    }
    property var allStories: [];
    property int cardWidth: 212;
    property int cardHeight: 152;
    property string metaverseBase: addressBarDialog.metaverseServerUrl + "/api/v1/";
    property bool isCursorVisible: false  // Override default cursor visibility.

    AddressBarDialog {
        id: addressBarDialog

        property bool keyboardEnabled: false
        property bool keyboardRaised: false
        property bool punctuationMode: false

        implicitWidth: backgroundImage.width
        implicitHeight: scroll.height + gap + backgroundImage.height + (keyboardEnabled ? keyboard.height : 0);

        // The buttons have their button state changed on hover, so we have to manually fix them up here
        onBackEnabledChanged: backArrow.buttonState = addressBarDialog.backEnabled ? 1 : 0;
        onForwardEnabledChanged: forwardArrow.buttonState = addressBarDialog.forwardEnabled ? 1 : 0;
        onReceivedHifiSchemeURL: resetAfterTeleport();

        // Update location after using back and forward buttons.
        onMetaverseServerUrlChanged: updateLocationTextTimer.start();

        ListModel { id: suggestions }

        ListView {
            id: scroll
            height: cardHeight + scroll.stackedCardShadowHeight
            property int stackedCardShadowHeight: 10;
            spacing: gap;
            clip: true;
            anchors {
                left: backgroundImage.left
                right: swipe.left
                bottom: backgroundImage.top
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
                thumbnail: model.thumbnail_url;
                action: model.action;
                timestamp: model.created_at;
                onlineUsers: model.online_users;
                storyId: model.metaverseId;
                drillDownToPlace: model.drillDownToPlace;
                shadowHeight: scroll.stackedCardShadowHeight;
                hoverThunk: function () { ListView.view.currentIndex = index; }
                unhoverThunk: function () { ListView.view.currentIndex = -1; }
            }
            highlightMoveDuration: -1;
            highlightMoveVelocity: -1;
            highlight: Rectangle { color: "transparent"; border.width: 4; border.color: hifiStyleConstants.colors.blueHighlight; z: 1; }
        }
        Image { // Just a visual indicator that the user can swipe the cards over to see more.
            id: swipe;
            source: "../images/swipe-chevron.svg";
            width: 72;
            visible: suggestions.count > 3;
            anchors {
                right: backgroundImage.right;
                top: scroll.top;
            }
            MouseArea {
                anchors.fill: parent
                onClicked: scroll.currentIndex = (scroll.currentIndex < 0) ? 3 : (scroll.currentIndex + 3)
            }
        }

        Row {
            spacing: 2 * hifi.layout.spacing;
            anchors {
                top: parent.top;
                left: parent.left;
                leftMargin: 150;
                topMargin: -30;
            }
            property var selected: allTab;
            TextButton {
                id: allTab;
                text: "ALL";
                property string includeActions: 'snapshot,concurrency';
                selected: allTab === selectedTab;
                action: tabSelect;
            }
            TextButton {
                id: placeTab;
                text: "PLACES";
                property string includeActions: 'concurrency';
                selected: placeTab === selectedTab;
                action: tabSelect;
            }
            TextButton {
                id: snapsTab;
                text: "SNAPS";
                property string includeActions: 'snapshot';
                selected: snapsTab === selectedTab;
                action: tabSelect;
            }
        }

        Image {
            id: backgroundImage
            source: "../images/address-bar-856.svg"
            width: 856
            height: 100
            anchors {
                bottom: parent.keyboardEnabled ? keyboard.top : parent.bottom;
            }
            property int inputAreaHeight: 70
            property int inputAreaStep: (height - inputAreaHeight) / 2

            ToolbarButton {
                id: homeButton
                imageURL: "../images/home.svg"
                buttonState: 1
                defaultState: 1
                hoverState: 2
                onClicked: {
                    addressBarDialog.loadHome();
                    root.shown = false;
                }
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

            HifiStyles.RalewayLight {
                id: notice;
                font.pixelSize: hifi.fonts.pixelSize * 0.50;
                anchors {
                    top: parent.top
                    topMargin: parent.inputAreaStep + 12
                    left: addressLine.left
                    right: addressLine.right
                }
            }
            HifiStyles.FiraSansRegular {
                id: location;
                font.pixelSize: addressLine.font.pixelSize;
                color: "gray";
                clip: true;
                anchors.fill: addressLine;
                visible: addressLine.text.length === 0
            }
            TextInput {
                id: addressLine
                focus: true
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: forwardArrow.right
                    right: parent.right
                    leftMargin: forwardArrow.width
                    rightMargin: forwardArrow.width / 2
                    topMargin: parent.inputAreaStep + (2 * hifi.layout.spacing)
                    bottomMargin: parent.inputAreaStep
                }
                font.pixelSize: hifi.fonts.pixelSize * 0.75
                cursorVisible: false
                onTextChanged: {
                    filterChoicesByText();
                    updateLocationText(text.length > 0);
                    if (!isCursorVisible && text.length > 0) {
                        isCursorVisible = true;
                        cursorVisible = true;
                    }
                }
                onActiveFocusChanged: {
                    cursorVisible = isCursorVisible && focus;
                }
                MouseArea {
                    // If user clicks in address bar show cursor to indicate ability to enter address.
                    anchors.fill: parent
                    onClicked: {
                        isCursorVisible = true;
                        parent.cursorVisible = true;
                        parent.forceActiveFocus();
                    }
                }
            }
        }

        Timer {
            // Delay updating location text a bit to avoid flicker of content and so that connection status is valid.
            id: updateLocationTextTimer
            running: false
            interval: 500  // ms
            repeat: false
            onTriggered: updateLocationText(false);
        }

        Timer {
            // Delay clearing address line so as to avoid flicker of "not connected" being displayed after entering an address.
            id: clearAddressLineTimer
            running: false
            interval: 100  // ms
            repeat: false
            onTriggered: {
                addressLine.text = "";
                isCursorVisible = false;
            }
        }

        Window {
            width: 938
            height: 625
            HifiControls.WebView {
                anchors.fill: parent;
                id: storyCardHTML;
            }
            id: storyCardFrame;

            shown: false;
            destroyOnCloseButton: false;
            pinnable: false;

            anchors {
                verticalCenter: backgroundImage.verticalCenter;
                horizontalCenter: scroll.horizontalCenter;
            }
            z: 100
        }

        HifiControls.Keyboard {
            id: keyboard
            raised: parent.keyboardEnabled  // Ignore keyboardRaised; keep keyboard raised if enabled (i.e., in HMD).
            numeric: parent.punctuationMode
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
        }
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
    function resolveUrl(url) {
        return (url.indexOf('/') === 0) ? (addressBarDialog.metaverseServerUrl + url) : url;
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
            created_at: data.created_at || "",
            action: data.action || "",
            thumbnail_url: resolveUrl(thumbnail_url),

            metaverseId: (data.id || "").toString(), // Some are strings from server while others are numbers. Model objects require uniformity.

            tags: tags,
            description: description,
            online_users: data.details.concurrency || 0,
            drillDownToPlace: false,

            searchText: [name].concat(tags, description || []).join(' ').toUpperCase()
        }
    }
    function suggestable(place) {
        if (place.action === 'snapshot') {
            return true;
        }
        return (place.place_name !== AddressManager.placename); // Not our entry, but do show other entry points to current domain.
    }
    property var selectedTab: allTab;
    function tabSelect(textButton) {
        selectedTab = textButton;
        fillDestinations();
    }
    property var placeMap: ({});
    function addToSuggestions(place) {
        var collapse = allTab.selected && (place.action !== 'concurrency');
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
    property int requestId: 0;
    function getUserStoryPage(pageNumber, cb) { // cb(error) after all pages of domain data have been added to model
        var options = [
            'now=' + new Date().toISOString(),
            'include_actions=' + selectedTab.includeActions,
            'restriction=' + (Account.isLoggedIn() ? 'open,hifi' : 'open'),
            'require_online=true',
            'protocol=' + encodeURIComponent(AddressManager.protocolVersion()),
            'page=' + pageNumber
        ];
        var url = metaverseBase + 'user_stories?' + options.join('&');
        var thisRequestId = ++requestId;
        getRequest(url, function (error, data) {
            if ((thisRequestId !== requestId) || handleError(url, error, data, cb)) {
                return;
            }
            var stories = data.user_stories.map(function (story) { // explicit single-argument function
                return makeModelData(story, url);
            });
            allStories = allStories.concat(stories);
            stories.forEach(makeFilteredPlaceProcessor());
            if ((data.current_page < data.total_pages) && (data.current_page <=  10)) { // just 10 pages = 100 stories for now
                return getUserStoryPage(pageNumber + 1, cb);
            }
            cb();
        });
    }
    function makeFilteredPlaceProcessor() { // answer a function(placeData) that adds it to suggestions if it matches
        var words = addressLine.text.toUpperCase().split(/\s+/).filter(identity),
            data = allStories;
        function matches(place) {
            if (!words.length) {
                return suggestable(place);
            }
            return words.every(function (word) {
                return place.searchText.indexOf(word) >= 0;
            });
        }
        return function (place) {
            if (matches(place)) {
                addToSuggestions(place);
            }
        };
    }
    function filterChoicesByText() {
        suggestions.clear();
        placeMap = {};
        allStories.forEach(makeFilteredPlaceProcessor());
    }

    function fillDestinations() {
        allStories = [];
        suggestions.clear();
        placeMap = {};
        getUserStoryPage(1, function (error) {
            console.log('user stories query', error || 'ok', allStories.length);
        });
    }

    function updateLocationText(enteringAddress) {
        if (enteringAddress) {
            notice.text = "Go to a place, @user, path or network address";
            notice.color = hifiStyleConstants.colors.baseGrayHighlight;
        } else {
            notice.text = AddressManager.isConnected ? "Your location:" : "Not Connected";
            notice.color = AddressManager.isConnected ? hifiStyleConstants.colors.baseGrayHighlight : hifiStyleConstants.colors.redHighlight;
            // Display hostname, which includes ip address, localhost, and other non-placenames.
            location.text = (AddressManager.placename || AddressManager.hostname || '') + (AddressManager.pathname ? AddressManager.pathname.match(/\/[^\/]+/)[0] : '');
        }
    }

    onVisibleChanged: {
        updateLocationText(false);
        if (visible) {
            addressLine.forceActiveFocus();
            fillDestinations();
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
                clearAddressLineTimer.start();
                event.accepted = true
                break
            case Qt.Key_Enter:
            case Qt.Key_Return:
                toggleOrGo()
                clearAddressLineTimer.start();
                event.accepted = true
                break
        }
    }
}
