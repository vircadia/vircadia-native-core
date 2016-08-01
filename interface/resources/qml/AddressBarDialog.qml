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
        addressLine.text = card.path;
        toggleOrGo(true);
    }
    property var allPlaces: [];
    property int cardWidth: 200;
    property int cardHeight: 152;

    AddressBarDialog {
        id: addressBarDialog
        implicitWidth: backgroundImage.width
        implicitHeight: backgroundImage.height

        ListModel { id: suggestions }

        ListView {
            width: (3 * cardWidth) + (2 * hifi.layout.spacing);
            height: cardHeight;
            spacing: hifi.layout.spacing;
            clip: true;
            anchors {
                bottom: backgroundImage.top;
                bottomMargin: 2 * hifi.layout.spacing;
                right: backgroundImage.right;
            }
            model: suggestions;
            orientation: ListView.Horizontal;
            delegate: Card {
                width: cardWidth;
                height: cardHeight;
                goFunction: goCard;
                path: model.name + model.path;
                thumbnail: model.thumbnail;
                placeText: model.name;
                usersText: model.online_users + ((model.online_users === 1) ? ' person' : ' people');
                hoverThunk: function () { ListView.view.currentIndex = index; }
                unhoverThunk: function () { ListView.view.currentIndex = -1; }
            }
            highlight: Rectangle { color: "transparent"; border.width: 2; border.color: "#1FA5E8"; z: 1; }
        }

        Image {
            id: backgroundImage
            source: "../images/address-bar.svg"
            width: 576 * root.scale
            height: 80 * root.scale
            property int inputAreaHeight: 56.0 * root.scale  // Height of the background's input area
            property int inputAreaStep: (height - inputAreaHeight) / 2

            Image {
                id: homeButton
                source: "../images/home-button.svg"
                width: 29
                height: 26
                anchors {
                    left: parent.left
                    leftMargin: parent.height + 2 * hifi.layout.spacing
                    verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadHome()
                    }
                }
            }

            Image {
                id: backArrow
                source: addressBarDialog.backEnabled ? "../images/left-arrow.svg" : "../images/left-arrow-disabled.svg"
                width: 22
                height: 26
                anchors {
                    left: homeButton.right
                    leftMargin: 2 * hifi.layout.spacing
                    verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadBack()
                    }
                }
            }

            Image {
                id: forwardArrow
                source: addressBarDialog.forwardEnabled ? "../images/right-arrow.svg" : "../images/right-arrow-disabled.svg"
                width: 22
                height: 26
                anchors {
                    left: backArrow.right
                    leftMargin: 2 * hifi.layout.spacing
                    verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadForward()
                    }
                }
            }

            // FIXME replace with TextField
            TextInput {
                id: addressLine
                focus: true
                anchors {
                    fill: parent
                    leftMargin: parent.height + parent.height + hifi.layout.spacing * 7
                    rightMargin: hifi.layout.spacing * 2
                    topMargin: parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + hifi.layout.spacing
                }
                font.pixelSize: hifi.fonts.pixelSize * root.scale * 0.75
                helperText: "Go to: place, @user, /path, network address"
                onTextChanged: filterChoicesByText()
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

    function handleError(error, data, cb) { // cb(error) and answer truthy if needed, else falsey
        if (!error && (data.status === 'success')) {
            return;
        }
        cb(error || new Error(data.status + ': ' + data.error));
        return true;
    }

    function getPlace(placeData, cb) { // cb(error, side-effected-placeData), after adding path, thumbnails, and description
        getRequest('https://metaverse.highfidelity.com/api/v1/places/' + placeData.name, function (error, data) {
            if (handleError(error, data, cb)) {
                return;
            }
            var place = data.data.place, previews = place.previews;
            placeData.path = place.path;
            if (previews && previews.thumbnail) {
                placeData.thumbnail = previews.thumbnail;
            }
            if (place.description) {
                placeData.description = place.description;
                placeData.searchText += ' ' + place.description.toUpperCase();
            }
            cb(error, placeData);
        });
    }
    function mapDomainPlaces(domain, cb) { // cb(error, arrayOfDomainPlaceData)
        function addPlace(name, icb) {
            getPlace({
                name: name,
                tags: domain.tags,
                thumbnail: "",
                description: "",
                path: "",
                searchText: [name].concat(domain.tags).join(' ').toUpperCase(),
                online_users: domain.online_users
            }, icb);
        }
        // IWBNI we could get these results in order with most-recent-entered first.
        // In any case, we don't really need to preserve the domain.names order in the results.
        asyncMap(domain.names, addPlace, cb);
    }

    function suggestable(place) {
        return (place.name !== AddressManager.hostname) // Not our entry, but do show other entry points to current domain.
            && place.thumbnail
            && place.online_users; // at least one present means it's actually online
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
        getRequest('https://metaverse.highfidelity.com/api/v1/domains/all?' + params.join('&'), function (error, data) {
            if (handleError(error, data, cb)) {
                return;
            }
            asyncMap(data.data.domains, mapDomainPlaces, function (error, pageResults) {
                if (error) {
                    return cb(error);
                }
                // pageResults is now [ [ placeDataOneForDomainOne, placeDataTwoForDomainOne, ...], [ placeDataTwoForDomainTwo...] ]
                pageResults.forEach(function (domainResults) {
                    allPlaces = allPlaces.concat(domainResults);
                    if (!addressLine.text) { // Don't add if the user is already filtering
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
    function filterChoicesByText() {
        suggestions.clear();
        var words = addressLine.text.toUpperCase().split(/\s+/).filter(identity);
        function matches(place) {
            if (!words.length) {
                return suggestable(place);
            }
            return words.every(function (word) {
                return place.searchText.indexOf(word) >= 0;
            });
        }
        allPlaces.forEach(function (place) {
            if (matches(place)) {
                suggestions.append(place);
            }
        });
    }

    function fillDestinations() {
        allPlaces = [];
        suggestions.clear();
        getDomainPage(1, function (error) {
            if (error) {
                console.log('domain query failed:', error);
            }
            console.log('domain query finished', allPlaces.length);
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
