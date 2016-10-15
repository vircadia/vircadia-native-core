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

    objectName: "AddressBarDialog"
    title: "Go To"

    shown: false
    destroyOnHidden: false
    resizable: false
    pinnable: false;
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
    property int keyboardHeight: 200;
    property int cardWidth: 200;
    property int cardHeight: 152;
    property string metaverseBase: addressBarDialog.metaverseServerUrl + "/api/v1/";
    property bool isCursorVisible: false  // Override default cursor visibility.

    AddressBarDialog {
        id: addressBarDialog

        property bool punctuationMode: false

        implicitWidth: backgroundImage.width
        implicitHeight: backgroundImage.height + keyboardHeight + cardHeight - 25; // fudge to make header reasonable

        // The buttons have their button state changed on hover, so we have to manually fix them up here
        onBackEnabledChanged: backArrow.buttonState = addressBarDialog.backEnabled ? 1 : 0;
        onForwardEnabledChanged: forwardArrow.buttonState = addressBarDialog.forwardEnabled ? 1 : 0;
        onReceivedHifiSchemeURL: resetAfterTeleport();

        // Update location after using back and forward buttons.
        onMetaverseServerUrlChanged: updateLocationTextTimer.start();

        ListModel { id: suggestions }

        ListView {
            id: scroll
            width: backgroundImage.width;
            height: cardHeight;
            spacing: hifi.layout.spacing;
            clip: true;
            anchors {
                bottom: backgroundImage.top;
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
                thumbnail: model.thumbnail_url;
                action: model.action;
                timestamp: model.created_at;
                onlineUsers: model.online_users;
                storyId: model.metaverseId;
                hoverThunk: function () { ListView.view.currentIndex = index; }
                unhoverThunk: function () { ListView.view.currentIndex = -1; }
            }
            highlightMoveDuration: -1;
            highlightMoveVelocity: -1;
            highlight: Rectangle { color: "transparent"; border.width: 4; border.color: "#1DB5ED"; z: 1; }
            leftMargin: 50; // Start the first item over by about the same amount as the last item peeks through on the other side.
            rightMargin: 50;
        }
        Image { // Just a visual indicator that the user can swipe the cards over to see more.
            source: "../images/Swipe-Icon-single.svg"
            width: 50;
            visible: suggestions.count > 3;
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
            anchors {
                verticalCenter: parent.verticalCenter;
                verticalCenterOffset: -15; // fudge to keep header reasonable and keep us under Card
            }
            property int inputAreaHeight: 56.0 * root.scale  // Height of the background's input area
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
                font.pixelSize: hifi.fonts.pixelSize * root.scale * 0.50;
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
                font.pixelSize: hifi.fonts.pixelSize * root.scale * 0.75
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
            scale: 0.8  // Reset scale of Window to 1.0 (counteract address bar's scale value of 1.25)
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
        }

        // virtual keyboard, letters
        HifiControls.Keyboard {
            id: keyboard1
            y: parent.height
            height: keyboardHeight
            visible: !parent.punctuationMode
            enabled: !parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.top: backgroundImage.bottom
            anchors.bottomMargin: 0
        }

        HifiControls.KeyboardPunctuation {
            id: keyboard2
            y: parent.height
            height: keyboardHeight
            visible: parent.punctuationMode
            enabled: parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.top: backgroundImage.bottom
            anchors.bottomMargin: 0
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

            searchText: [name].concat(tags, description || []).join(' ').toUpperCase()
        }
    }
    function suggestable(place) {
        if (place.action === 'snapshot') {
            return true;
        }
        return (place.place_name !== AddressManager.hostname); // Not our entry, but do show other entry points to current domain.
        // could also require right protocolVersion
    }
    function getUserStoryPage(pageNumber, cb) { // cb(error) after all pages of domain data have been added to model
        var options = [
            'include_actions=snapshot,concurrency',
            'protocol=' + encodeURIComponent(AddressManager.protocolVersion()),
            'page=' + pageNumber
        ];
        var url = metaverseBase + 'user_stories?' + options.join('&');
        getRequest(url, function (error, data) {
            if (handleError(url, error, data, cb)) {
                return;
            }
            var stories = data.user_stories.map(function (story) { // explicit single-argument function
                return makeModelData(story, url);
            });
            allStories = allStories.concat(stories);
            if (!addressLine.text) { // Don't add if the user is already filtering
                stories.forEach(function (story) {
                    if (suggestable(story)) {
                        suggestions.append(story);
                    }
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
            data = allStories;
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
        allStories = [];
        suggestions.clear();
        getUserStoryPage(1, function (error) {
            console.log('user stories query', error || 'ok', allStories.length);
        });
    }

    function updateLocationText(enteringAddress) {
        if (enteringAddress) {
            notice.text = "Go to a place, @user, path or network address";
            notice.color = "gray";
        } else {
            notice.text = AddressManager.isConnected ? "Your location:" : "Not Connected";
            notice.color = AddressManager.isConnected ? "gray" : "crimson";
            // Display hostname, which includes ip address, localhost, and other non-placenames.
            location.text = (AddressManager.hostname || '') + (AddressManager.pathname ? AddressManager.pathname.match(/\/[^\/]+/)[0] : '');
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
