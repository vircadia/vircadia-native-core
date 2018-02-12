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
            updateLocationText(false);
        }
    }

    function hide() {
        shown = false;
        sendToScript ({ type: "hide" });
    }

    Component.onCompleted: {
        updateLocationText(false);
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
            font.pixelSize: hifi.fonts.pixelSize * 2.15;
            color: "#2CD7FF"
            anchors {
                bottom: addressBackground.top
                bottomMargin: 45
                left: addressBackground.left
                leftMargin: 60
            }

        }

        property int inputAreaHeight: 210
        property int inputAreaStep: (height - inputAreaHeight) / 2

        ToolbarButton {
            id: homeButton
            y: 280
            imageURL: "../../icons/home.svg"
            onClicked: {
                addressBarDialog.loadHome();
                bar.shown = false;
            }
            anchors {
                leftMargin: 75
                left: parent.left
            }
            size: 150
        }

        ToolbarButton {
            id: backArrow;
            imageURL: "../../icons/backward.svg";
            onClicked: addressBarDialog.loadBack();
            anchors {
                left: homeButton.right
                leftMargin: 70
                verticalCenter: homeButton.verticalCenter
            }
            size: 150
        }
        ToolbarButton {
            id: forwardArrow;
            imageURL: "../../icons/forward.svg";
            onClicked: addressBarDialog.loadForward();
            anchors {
                left: backArrow.right
                leftMargin: 60
                verticalCenter: homeButton.verticalCenter
            }
            size: 150
        }

        HifiStyles.FiraSansRegular {
            id: location;
            font.pixelSize: addressLine.font.pixelSize;
            color: "gray";
            clip: true;
            anchors.fill: addressLine;
            visible: addressLine.text.length === 0
            z: 1
        }

        Rectangle {
            id: addressBackground
            x: 780
            y: 280
            width: 1440
            height: 150
            color: "#FFFFFF"
        }

        TextInput {
            id: addressLine
            focus: true
            x: 870
            y: 450
            width: 1350
            height: 120
            inputMethodHints: Qt.ImhNoPredictiveText
            //helperText: "Hint is here"
            anchors {
                verticalCenter: homeButton.verticalCenter                
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