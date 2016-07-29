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
    property var allDomains: [];
    property var suggestionChoices: [];
    property var domainsBaseUrl: null;
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
                path: model.name;
                thumbnail: model.thumbnail;
                placeText: model.name;
                usersText: model.online_users + ((model.online_users === 1) ? ' person' : ' people');
            }
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
    // call iterator(element, icb) once for each element of array, and then cb(error) when icb(error) has been called by each iterator.
    // short-circuits if error. Note that iterator MUST be an asynchronous function. (Use setTimeout if necessary.)
    function asyncEach(array, iterator, cb) {
        var count = array.length;
        function icb(error) {
            if (!--count || error) {
                count = -1; // don't cb multiple times (e.g., if error)
                cb(error);
            }
        }
        if (!count) {
            return cb();
        }
        array.forEach(function (element) {
            iterator(element, icb);
        });
    }

    function identity(x) {
        return x;
    }

    function addPictureToDomain(domainInfo, cb) { // asynchronously add thumbnail and lobby to domainInfo, if available, and cb(error)
        // This requests data for all the names at once, and just uses the first one to come back.
        // We might change this to check one at a time, which would be less requests and more latency.
        domainInfo.thumbnail = ''; // Regardless of whether we fill it in later, qt models must start with the all values they will have.
        asyncEach([domainInfo.name].concat(domainInfo.names || null).filter(identity), function (name, icb) {
            var url = "https://metaverse.highfidelity.com/api/v1/places/" + name;
            getRequest(url, function (error, json) {
                var previews = !error && json.data.place.previews;
                if (previews) {
                    if (!domainInfo.thumbnail) { // just grab the first one
                        domainInfo.thumbnail = previews.thumbnail;
                    }
                    if (!domainInfo.lobby) {
                        domainInfo.lobby = previews.lobby;
                    }
                }
                icb(error);
            });
        }, cb);
    }

    function getDomains(options, cb) { // cb(error, arrayOfData)
        if (!options.page) {
            options.page = 1;
        }
        if (!domainsBaseUrl) {
            var domainsOptions = [
                'open', // published hours handle now
                // fixme hrs restore 'active', // has at least one person connected. FIXME: really want any place that is verified accessible.
                // FIXME: really want places I'm allowed in, not just open ones.
                'restriction=open', // Not by whitelist, etc.  FIXME: If logged in, add hifi to the restriction options, in order to include places that require login.
                // FIXME add maturity
                'protocol=' + encodeURIComponent(AddressManager.protocolVersion()),
                'sort_by=users',
                'sort_order=desc',
            ];
            domainsBaseUrl = "https://metaverse.highfidelity.com/api/v1/domains/all?" + domainsOptions.join('&');
        }
        var url = domainsBaseUrl + "&page=" + options.page + "&users=" + options.minUsers + "-" + options.maxUsers;
        getRequest(url, function (error, json) {
            if (!error && (json.status !== 'success')) {
                error = new Error("Bad response: " + JSON.stringify(json));
            }
            if (error) {
                error.message += ' for ' + url;
                return cb(error);
            }
            var domains = json.data.domains;
            if (json.current_page < json.total_pages) {
                options.page++;
                return getDomains(options, function (error, others) {
                    cb(error, domains.concat(others));
                });
            }
            cb(null, domains);
        });
    }

    function filterChoicesByText() {
        function fill1(targetIndex) {
            var data = filtered[targetIndex];
            if (!data) {
                if (targetIndex < suggestions.count) {
                    suggestions.remove(targetIndex);
                }
                return;
            }
            console.log('suggestion:', JSON.stringify(data));
            if (suggestions.count <= targetIndex) {
                suggestions.append(data);
            } else {
                suggestions.set(targetIndex, data);
            }
        }
        var words = addressLine.text.toUpperCase().split(/\s+/).filter(identity);
        var filtered = !words.length ? suggestionChoices : allDomains.filter(function (domain) {
            var text = domain.names.concat(domain.tags).join(' ');
            if (domain.description) {
                text += domain.description;
            }
            text = text.toUpperCase();
            return words.every(function (word) {
                return text.indexOf(word) >= 0;
            });
        });
        for (var index in filtered) { fill1(index); }
    }

    function fillDestinations() {
        allDomains = suggestionChoices = [];
        getDomains({minUsers: 0, maxUsers: 20}, function (error, domains) {
            if (error) {
                console.log('domain query failed:', error);
                return filterChoicesByText();
            }
            var here = AddressManager.hostname; // don't show where we are now.
            allDomains = domains.filter(function (domain) { return domain.name !== here; });
            // Whittle down suggestions to those that have at least one user, and try to get pictures.
            suggestionChoices = allDomains.filter(function (domain) { return true/*fixme hrs restore domain.online_users*/; });
            asyncEach(domains, addPictureToDomain, function (error) {
                if (error) {
                    console.log('place picture query failed:', error);
                }
                // Whittle down more by requiring a picture.
                // fixme hrs restore suggestionChoices = suggestionChoices.filter(function (domain) { return domain.lobby; });
                filterChoicesByText();
            });
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
