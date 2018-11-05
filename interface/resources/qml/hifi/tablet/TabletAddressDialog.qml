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
import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import "../../controls"
import "../../styles"
import "../../windows"
import "../"
import "../toolbars"
import "../../styles-uit" as HifiStyles
import "../../controls-uit" as HifiControls
import QtQuick.Controls 2.2 as QQC2
import QtQuick.Templates 2.2 as T

// references HMD, AddressManager, AddressBarDialog from root context

StackView {
    id: root;
    HifiConstants { id: hifi }
    HifiStyles.HifiConstants { id: hifiStyleConstants }
    initialItem: addressBarDialog
    width: parent !== null ? parent.width : undefined
    height: parent !== null ? parent.height : undefined
    property int cardWidth: 212;
    property int cardHeight: 152;
    property var tablet: null;

    RootHttpRequest { id: http; }
    signal sendToScript(var message);
    function fromScript(message) {
        switch (message.method) {
        case 'http.response':
            http.handleHttpResponse(message);
            break;
        }
    }

    Component { id: tabletWebView; TabletWebView {} }
    Component.onCompleted: {
        updateLocationText(false);
        root.parentChanged.connect(center);
        center();
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

        Qt.callLater(function() {
            addressBarDialog.keyboardEnabled = HMD.active;
            addressLine.forceActiveFocus();
        })
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
            root.push(card);
            return;
        }
        location.text = targetString;
        toggleOrGo(targetString, true);
        clearAddressLineTimer.start();
    }


    AddressBarDialog {
        id: addressBarDialog

        property bool keyboardEnabled: false
        property bool punctuationMode: false
        property bool keyboardRaised: false

        width: parent.width
        height: parent.height

        MouseArea {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                bottom: keyboard.top
            }

            propagateComposedEvents: true
            onPressed: {
                parent.forceActiveFocus();
                mouse.accepted = false;
            }
        }

        anchors {
            right: parent.right
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }

        onHostChanged: {
            updateLocationTextTimer.restart();
            DialogsManager.hideAddressBar();
        }

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

            HifiStyles.RalewayRegular {
                id: notice;
                font.pixelSize: hifi.fonts.pixelSize * 0.7;
                anchors {
                    top: parent.top;
                    left: addressLineContainer.left;
                }
            }

            HifiStyles.FiraSansRegular {
                id: location;
                anchors {
                    left: notice.right
                    leftMargin: 8
                    right: addressLineContainer.right
                    verticalCenter: notice.verticalCenter
                }
                font.pixelSize: addressLine.font.pixelSize;
                color: "gray";
                clip: true;
                visible: addressLine.text.length === 0
            }

            QQC2.TextField {
                id: addressLine

                focus: true
                width: addressLineContainer.width - addressLineContainer.anchors.leftMargin - addressLineContainer.anchors.rightMargin;
                anchors {
                    left: addressLineContainer.left;
                    leftMargin: 8;
                    verticalCenter: addressLineContainer.verticalCenter;
                }
                onTextChanged: {
                    updateLocationText(text.length > 0);
                }
                onAccepted: {
                    toggleOrGo();
                }

                // unfortunately TextField from Quick Controls 2 disallow customization of placeHolderText color without creation of new style
                property string placeholderText2: "Type domain address here"
                verticalAlignment: TextInput.AlignBottom

                font {
                    family: hifi.fonts.fontFamily
                    pixelSize: hifi.fonts.pixelSize * 0.75
                }

                color: hifi.colors.text
                background: Item {}

            }

            QQC2.Label {
                font: addressLine.font

                x: addressLine.x
                y: addressLine.y
                leftPadding: addressLine.leftPadding
                topPadding: addressLine.topPadding

                text: addressLine.placeholderText2
                verticalAlignment: "AlignVCenter"
                color: 'gray'
                visible: addressLine.text === ''
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
                        addressLine.focus = true;
                        addressLine.forceActiveFocus();
                        if (HMD.active) {
                            addressBarDialog.keyboardEnabled = HMD.active;
                        }
                        tabletRoot.playButtonClickSound();
                    }
                }
            }
        }

        Rectangle {
            id: bgMain;
            anchors {
                top: addressBar.bottom;
                bottom: parent.keyboardEnabled ? keyboard.top : parent.bottom;
                left: parent.left;
                right: parent.right;
            }
            Rectangle {
                id: addressShadow;
                width: parent.width;
                height: 42 - 33;
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "gray" }
                    GradientStop { position: 1.0; color: "white" }
                }
            }
            Rectangle { // Column margins require QtQuick 2.7, which we don't use yet.
                id: column;
                property real pad: 10;
                width: bgMain.width - column.pad;
                height: stack.height;
                color: "transparent";
                anchors {
                    left: parent.left;
                    leftMargin: column.pad;
                    top: addressShadow.bottom;
                    topMargin: column.pad;
                }
                Column {
                    id: stack;
                    width: column.width;
                    spacing: 33 - places.labelSize;
                    Feed {
                        id: happeningNow;
                        width: parent.width;
                        cardWidth: 312 + (2 * 4);
                        cardHeight: 163 + (2 * 4);
                        labelText: 'FEATURED';
                        actions: 'announcement';
                        filter: addressLine.text;
                        goFunction: goCard;
                        http: http;
                        autoScrollTimerEnabled: true;
                    }
                    Feed {
                        id: places;
                        width: parent.width;
                        cardWidth: 210;
                        cardHeight: 110 + messageHeight;
                        messageHeight: 44;
                        labelText: 'PLACES';
                        actions: 'concurrency';
                        filter: addressLine.text;
                        goFunction: goCard;
                        http: http;
                    }
                    Feed {
                        id: snapshots;
                        width: parent.width;
                        cardWidth: 143 + (2 * 4);
                        cardHeight: 75 + messageHeight + 4;
                        messageHeight: 32;
                        textPadding: 6;
                        labelText: 'RECENT SNAPS';
                        actions: 'snapshot';
                        filter: addressLine.text;
                        goFunction: goCard;
                        http: http;
                    }
                }
            }
        }

        Timer {
            // Delay updating location text a bit to avoid flicker of content and so that connection status is valid.
            id: updateLocationTextTimer
            running: false
            interval: 1000  // ms
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
            raised: parent.keyboardEnabled && parent.keyboardRaised
            numeric: parent.punctuationMode
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
        }

    }

    function updateLocationText(enteringAddress) {
        if (enteringAddress) {
            notice.text = "Go To a place, @user, path, or network address:";
            notice.color = hifiStyleConstants.colors.baseGrayHighlight;
        } else {
            notice.text = AddressManager.isConnected ? "YOUR LOCATION" : "NOT CONNECTED";
            notice.color = AddressManager.isConnected ? hifiStyleConstants.colors.blueHighlight : hifiStyleConstants.colors.redHighlight;
            // Display hostname, which includes ip address, localhost, and other non-placenames.
            location.text = (AddressManager.placename || AddressManager.hostname || '') + (AddressManager.pathname ? AddressManager.pathname.match(/\/[^\/]+/)[0] : '');
        }
    }

    function toggleOrGo(address, fromSuggestions) {
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
