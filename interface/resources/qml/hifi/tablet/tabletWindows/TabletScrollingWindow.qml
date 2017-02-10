//
//  TabletsScrollingWindow.qml
//
//  Created by Dante Ruiz on 9 Feb 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0

import "."
import "../../../styles-uit"
import "../../../controls-uit" as HiFiControls
FocusScope{
    id: window
    HifiConstants { id: hifi }

    childern [ plane ]
    property var footer: Item {}
    readonly var footerContentHeight: 40
    property var pane: Item {
        property bool isScrolling: true//scrollView.height < scrollView.contentItem.height
        property int contentWidth: scrollView.width - (isScrolling ? 10 : 0)
        property int scrollHeight: scrollView.height
        anchors.fill: parent
        anchors.rightMargin: 11
        Rectangle {
            id: contentBackground
            anchors.fill: parent
            color: hifi.colors.baseGray
            visible: true
        }

         LinearGradient {
            visible: true
            anchors.top: contentBackground.bottom
            anchors.left: contentBackground.left
            width: contentBackground.width - 1
            height: 4
            start: Qt.point(0, 0)
            end: Qt.point(0, 4)
            gradient: Gradient {
                GradientStop { position: 0.0; color: hifi.colors.darkGray }
                GradientStop { position: 1.0; color: hifi.colors.darkGray0 }
            }
            cached: true
        }

         ScrollView {
            id: scrollView
            horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
            verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
            anchors.fill: parent
            anchors.rightMargin: parent.isScrolling ? 1 : 0
            anchors.bottomMargin: footerPane.height

            style: ScrollViewStyle {

                padding.right: -7  // Move to right away from content.

                handle: Item {
                    implicitWidth: 8
                    Rectangle {
                        radius: 4
                        color: hifi.colors.white30
                        anchors {
                            fill: parent
                            leftMargin: 2  // Finesse size and position.
                            topMargin: 1
                            bottomMargin: 1
                        }
                    }
                }

                scrollBarBackground: Item {
                    implicitWidth: 10
                    Rectangle {
                        color: hifi.colors.darkGray30
                        radius: 4
                        anchors {
                            fill: parent
                            topMargin: -1  // Finesse size
                            bottomMargin: -2
                        }
                    }
                }

                incrementControl: Item {
                    visible: false
                }

                decrementControl: Item {
                    visible: false
                }
            }
         }

        function scrollBy(delta) {
            scrollView.flickableItem.contentY += delta;
        }

        Rectangle {
            // Optional non-scrolling footer.
            id: footerPane

            property alias keyboardOverride: window.keyboardOverride
            property alias keyboardRaised: window.keyboardRaised
            property alias punctuationMode: window.punctuationMode

            anchors {
                left: parent.left
                bottom: parent.bottom
            }
            width: parent.contentWidth
            height: footerContentHeight + (keyboard.enabled && keyboard.raised ? keyboard.height : 0)
            color: hifi.colors.baseGray
            visible: footer.height > 0 || keyboard.enabled && keyboard.raised

            Item {
                // Horizontal rule.
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }

                visible: footer.height > 0

                Rectangle {
                    width: parent.width
                    height: 1
                    y: 1  // Stop displaying content just above horizontal rule/=.
                    color: hifi.colors.baseGrayShadow
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    y: 2
                    color: hifi.colors.baseGrayHighlight
                }
            }

            Item {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    topMargin: hifi.dimensions.contentSpacing.y + 3
                }
                children: [ footer ]
            }

            HiFiControls.Keyboard {
                id: keyboard
                enabled: !keyboardOverride
                raised: keyboardEnabled && keyboardRaised
                numeric: punctuationMode
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
            }
        }
    }
}
