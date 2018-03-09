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
import "../controls"
import "../styles"
import "../hifi" as QmlHifi
import "../hifi/toolbars"
import "../styles-uit" as HifiStyles
import "../controls-uit" as HifiControls

Item {
    QmlHifi.HifiConstants { id: android }

    width: parent ? parent.width - android.dimen.windowLessWidth : 0
    height: parent ? parent.height - android.dimen.windowLessHeight : 0
    z: android.dimen.windowZ
    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }

    id: bar
    property bool isCursorVisible: false  // Override default cursor visibility.
    property bool shown: true

    onShownChanged: {
        bar.visible = shown;
        sendToScript({method: 'shownChanged', params: { shown: shown }});
        if (shown) {
            addressLine.text="";
            updateLocationText(addressLine.text.length > 0);
        }
    }

    function hide() {
        shown = false;
        sendToScript ({ type: "hide" });
    }

    Component.onCompleted: {
        updateLocationText(addressLine.text.length > 0);
    }

    HifiConstants { id: hifi }
    HifiStyles.HifiConstants { id: hifiStyleConstants }

    signal sendToScript(var message);

    AddressBarDialog {
        id: addressBarDialog
    }


    Rectangle {
        id: background
        gradient: Gradient {
            GradientStop { position: 0.0; color: android.color.gradientTop }
            GradientStop { position: 1.0; color: android.color.gradientBottom }
        }
        anchors {
            fill: parent
        }

        QmlHifi.WindowHeader {
            id: header
            iconSource: "../../../icons/goto-i.svg"
            titleText: "GO TO"
        }

        HifiStyles.RalewayRegular {
            id: notice
            text: "YOUR LOCATION"
            font.pixelSize: (hifi.fonts.pixelSize * 2.15)*(android.dimen.atLeast1440p?1:.75);
            color: "#2CD7FF"
            anchors {
                bottom: addressBackground.top
                bottomMargin: android.dimen.atLeast1440p?45:34
                left: addressBackground.left
                leftMargin: android.dimen.atLeast1440p?60:45
            }

        }

        property int inputAreaHeight: android.dimen.atLeast1440p?210:156
        property int inputAreaStep: (height - inputAreaHeight) / 2

        ToolbarButton {
            id: homeButton
            y: android.dimen.atLeast1440p?280:210
            imageURL: "../../icons/home.svg"
            onClicked: {
                addressBarDialog.loadHome();
                bar.shown = false;
            }
            anchors {
                leftMargin: android.dimen.atLeast1440p?75:56
                left: parent.left
            }
            size: android.dimen.atLeast1440p?150:150//112
        }

        ToolbarButton {
            id: backArrow;
            imageURL: "../../icons/backward.svg";
            onClicked: addressBarDialog.loadBack();
            anchors {
                left: homeButton.right
                leftMargin: android.dimen.atLeast1440p?70:52
                verticalCenter: homeButton.verticalCenter
            }
            size: android.dimen.atLeast1440p?150:150
        }
        ToolbarButton {
            id: forwardArrow;
            imageURL: "../../icons/forward.svg";
            onClicked: addressBarDialog.loadForward();
            anchors {
                left: backArrow.right
                leftMargin: android.dimen.atLeast1440p?60:45
                verticalCenter: homeButton.verticalCenter
            }
            size: android.dimen.atLeast1440p?150:150
        }

        HifiStyles.FiraSansRegular {
            id: location;
            font.pixelSize: addressLine.font.pixelSize;
            color: "lightgray";
            clip: true;
            anchors.fill: addressLine;
            visible: addressLine.text.length === 0
            z: 1
        }

        Rectangle {
            id: addressBackground
            x: android.dimen.atLeast1440p?780:585
            y: android.dimen.atLeast1440p?280:235 // tweaking by hand
            width: android.dimen.atLeast1440p?1270:952
            height: android.dimen.atLeast1440p?150:112
            color: "#FFFFFF"
        }

        TextInput {
            id: addressLine
            focus: true
            x: android.dimen.atLeast1440p?870:652
            y: android.dimen.atLeast1440p?300:245 // tweaking by hand
            width: android.dimen.atLeast1440p?1200:900
            height: android.dimen.atLeast1440p?120:90
            inputMethodHints: Qt.ImhNoPredictiveText
            //helperText: "Hint is here"
            anchors {
                //verticalCenter: addressBackground.verticalCenter
            }
            font.pixelSize: hifi.fonts.pixelSize * 3.75
            onTextChanged: {
                //filterChoicesByText();
                updateLocationText(addressLine.text.length > 0);
                if (!isCursorVisible && text.length > 0) {
                    isCursorVisible = true;
                    cursorVisible = true;
                }
            }

            onActiveFocusChanged: {
                //cursorVisible = isCursorVisible && focus;
            }
        }
        


        function toggleOrGo() {
            if (addressLine.text !== "") {
                addressBarDialog.loadAddress(addressLine.text);
            }
            bar.shown = false;
        }

        Keys.onPressed: {
            switch (event.key) {
                case Qt.Key_Escape:
                case Qt.Key_Back:
                    clearAddressLineTimer.start();
                    event.accepted = true
                    bar.shown = false;
                    break
                case Qt.Key_Enter:
                case Qt.Key_Return:
                    toggleOrGo();
                    clearAddressLineTimer.start();
                    event.accepted = true
                    break
            }
        }

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

    function updateLocationText(enteringAddress) {
        if (enteringAddress) {
            notice.text = "Go to a place, @user, path or network address";
            notice.color = "#ffffff"; // hifiStyleConstants.colors.baseGrayHighlight;
            location.visible = false;
        } else {
            notice.text = AddressManager.isConnected ? "YOUR LOCATION:" : "NOT CONNECTED";
            notice.color = AddressManager.isConnected ? hifiStyleConstants.colors.blueHighlight : hifiStyleConstants.colors.redHighlight;
            // Display hostname, which includes ip address, localhost, and other non-placenames.
            location.text = (AddressManager.placename || AddressManager.hostname || '') + (AddressManager.pathname ? AddressManager.pathname.match(/\/[^\/]+/)[0] : '');
            location.visible = true;
        }
    }

}