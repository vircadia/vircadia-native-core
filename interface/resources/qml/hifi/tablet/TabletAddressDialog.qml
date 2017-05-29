//
//  TabletAddressDialog.qml
//
//  Created by Dante Ruiz on 2017/03/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import "../../controls"
import "../../styles"
import "../../windows"
import "../"
import "../toolbars"
import "../../styles-uit" as HifiStyles
import "../../controls-uit" as HifiControls

// references HMD, AddressManager, AddressBarDialog from root context

StackView {
    id: root;
    HifiConstants { id: hifi }
    HifiStyles.HifiConstants { id: hifiStyleConstants }
    initialItem: addressBarDialog
    width: parent !== null ? parent.width : undefined
    height: parent !== null ? parent.height : undefined
    property var eventBridge;
    property var allStories: [];
    property int cardWidth: 460;
    property int cardHeight: 320;
    property string metaverseBase: addressBarDialog.metaverseServerUrl + "/api/v1/";

    property var tablet: null;

    Component { id: tabletWebView; TabletWebView {} }
    Component.onCompleted: {
        fillDestinations();
        updateLocationText(false);
        fillDestinations();
        addressLine.focus = !HMD.active;
        root.parentChanged.connect(center);
        center();
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    }
    Component.onDestruction: {
        root.parentChanged.disconnect(center);
    }

    function center() {
        // Explicitly center in order to avoid warnings at shutdown
        anchors.centerIn = parent;
    }


     function resetAfterTeleport() {
        //storyCardFrame.shown = root.shown = false;
    }
    function goCard(targetString) {
        if (0 !== targetString.indexOf('hifi://')) {
            var card = tabletWebView.createObject();
            card.url = addressBarDialog.metaverseServerUrl + targetString;
            card.parentStackItem = root;
            card.eventBridge = root.eventBridge;
            root.push(card);
            return;
        }
        location.text = targetString;
        toggleOrGo(true, targetString);
        clearAddressLineTimer.start();
    }


    AddressBarDialog {
        id: addressBarDialog

        property bool keyboardEnabled: false
        property bool keyboardRaised: false
        property bool punctuationMode: false
        
        width: parent.width
        height: parent.height

        anchors {
            right: parent.right
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }

        onMetaverseServerUrlChanged: updateLocationTextTimer.start();
        Rectangle {
            id: navBar
            width: parent.width
            height: 50;
            color: hifiStyleConstants.colors.white
            anchors.top: parent.top;
            anchors.left: parent.left;

            ToolbarButton {
                id: homeButton
                imageURL: "../../../images/home.svg"
                onClicked: {
                    addressBarDialog.loadHome();
                    tabletRoot.shown = false;
                    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
                    tablet.gotoHomeScreen();
                }
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                }
            }
            ToolbarButton {
                id: backArrow;
                buttonState: addressBarDialog.backEnabled;
                imageURL: "../../../images/backward.svg";
                buttonEnabled: addressBarDialog.backEnabled;
                onClicked: {
                    if (buttonEnabled) {
                        addressBarDialog.loadBack();
                    }
                }
                anchors {
                    left: homeButton.right
                    verticalCenter: parent.verticalCenter
                }
            }
            ToolbarButton {
                id: forwardArrow;
                buttonState: addressBarDialog.forwardEnabled;
                imageURL: "../../../images/forward.svg";
                buttonEnabled: addressBarDialog.forwardEnabled;
                onClicked: {
                    if (buttonEnabled) {
                        addressBarDialog.loadForward();
                    }
                }
                anchors {
                    left: backArrow.right
                    verticalCenter: parent.verticalCenter
                }
            }
        }

        Rectangle {
            id: addressBar
            width: parent.width
            height: 70
            color: hifiStyleConstants.colors.white
            anchors {
                top: navBar.bottom;
                left: parent.left;
            }

            HifiStyles.RalewayLight {
                id: notice;
                font.pixelSize: hifi.fonts.pixelSize * 0.7;
                anchors {
                    top: parent.top;
                    left: addressLineContainer.left;
                    right: addressLineContainer.right;
                }
            }

            HifiStyles.FiraSansRegular {
                id: location;
                anchors {
                    left: addressLineContainer.left;
                    leftMargin: 8;
                    verticalCenter: addressLineContainer.verticalCenter;
                }
                font.pixelSize: addressLine.font.pixelSize;
                color: "gray";
                clip: true;
                visible: addressLine.text.length === 0
            }

            TextInput {
                id: addressLine
                width: addressLineContainer.width - addressLineContainer.anchors.leftMargin - addressLineContainer.anchors.rightMargin;
                anchors {
                    left: addressLineContainer.left;
                    leftMargin: 8;
                    verticalCenter: addressLineContainer.verticalCenter;
                }
                font.pixelSize: hifi.fonts.pixelSize * 0.75
                onTextChanged: {
                    filterChoicesByText();
                    updateLocationText(text.length > 0);
                }
                onAccepted: {
                    addressBarDialog.keyboardEnabled = false;
                    toggleOrGo();
                }
            }

            Rectangle {
                id: addressLineContainer;
                height: 40;
                anchors {
                    top: notice.bottom;
                    topMargin: 2;
                    left: parent.left;
                    leftMargin: 16;
                    right: parent.right;
                    rightMargin: 16;
                }
                color: hifiStyleConstants.colors.lightGray
                opacity: 0.1
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        if (!addressLine.focus || !HMD.active) {
                            addressLine.focus = true;
                            addressLine.forceActiveFocus();
                            addressBarDialog.keyboardEnabled = HMD.active;
                        }
                        tabletRoot.playButtonClickSound();
                    }
                }
            }
        }
        Rectangle {
            id: topBar
            height: 37
            color: hifiStyleConstants.colors.white

            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.topMargin: 0
            anchors.top: addressBar.bottom
            
            Row {
                id: thing
                spacing: 5 * hifi.layout.spacing

                anchors {
                    top: parent.top;
                    left: parent.left
                    leftMargin: 25
                }

                TabletTextButton {
                    id: allTab;
                    text: "ALL";
                    property string includeActions: 'snapshot,concurrency';
                    selected: allTab === selectedTab;
                    action: tabSelect;
                }

                TabletTextButton {
                    id: placeTab;
                    text: "PLACES";
                    property string includeActions: 'concurrency';
                    selected: placeTab === selectedTab;
                    action: tabSelect;
                    
                }

                TabletTextButton {
                    id: snapTab;
                    text: "SNAP";
                    property string includeActions: 'snapshot';
                    selected: snapTab === selectedTab;
                    action: tabSelect;
                }
            }
        
        }

        Rectangle {
            id: bgMain
            color: hifiStyleConstants.colors.white
            anchors.bottom: parent.keyboardEnabled ? keyboard.top : parent.bottom
            anchors.bottomMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.top: topBar.bottom
            anchors.topMargin: 0

            ListModel { id: suggestions }

            ListView {
                id: scroll
                
                property int stackedCardShadowHeight: 0;
                clip: true
                spacing: 14
                anchors {
                    bottom: parent.bottom
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    leftMargin: 10
                }

                model: suggestions
                orientation: ListView.Vertical

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
                    shadowHeight: scroll.stackedCardShadowHeight;
                    hoverThunk: function () { scroll.currentIndex = index; }
                    unhoverThunk: function () { scroll.currentIndex = -1; }
                }

                highlightMoveDuration: -1;
                highlightMoveVelocity: -1;
                highlight: Rectangle { color: "transparent"; border.width: 4; border.color: hifiStyleConstants.colors.blueHighlight; z: 1; }
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
            id: clearAddressLineTimer;
            running: false;
            interval: 100;  // ms
            repeat: false;
            onTriggered: {
                addressLine.text = "";
            }
        }
           

        HifiControls.Keyboard {
            id: keyboard
            raised: parent.keyboardEnabled
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
            image_url: resolveUrl(data.details.image_url),

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
            notice.text = "Go To a place, @user, path, or network address:";
            notice.color = hifiStyleConstants.colors.baseGrayHighlight;
        } else {
            notice.text = AddressManager.isConnected ? "Your location:" : "Not Connected";
            notice.color = AddressManager.isConnected ? hifiStyleConstants.colors.baseGrayHighlight : hifiStyleConstants.colors.redHighlight;
            // Display hostname, which includes ip address, localhost, and other non-placenames.
            location.text = (AddressManager.placename || AddressManager.hostname || '') + (AddressManager.pathname ? AddressManager.pathname.match(/\/[^\/]+/)[0] : '');
        }
    }

    function toggleOrGo(fromSuggestions, address) {
        if (address !== undefined && address !== "") {
            addressBarDialog.loadAddress(address, fromSuggestions);
            clearAddressLineTimer.start();
        } else if (addressLine.text !== "") {
            addressBarDialog.loadAddress(addressLine.text, fromSuggestions);
            clearAddressLineTimer.start();
        }
        DialogsManager.hideAddressBar();
    }
}
