//
//  Window.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
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
import "../styles-uit"

// FIXME how do I set the initial position of a window without
// overriding places where the a individual client of the window
// might be setting the position with a Settings{} element?

// FIXME how to I enable dragging without allowing the window to lay outside
// of the desktop?  How do I ensure when the desktop resizes all the windows
// are still at least partially visible?
Fadable {
    id: window
    HifiConstants { id: hifi }

    // The Window size is the size of the content, while the frame
    // decorations can extend outside it.
    implicitHeight: content ? content.height : 0
    implicitWidth: content ? content.width : 0
    x: -1; y: -1
    enabled: visible

    signal windowDestroyed();

    property int modality: Qt.NonModal
    readonly property bool topLevelWindow: true
    property string title
    // Should the window be closable control?
    property bool closable: true
    // Should the window try to remain on top of other windows?
    property bool alwaysOnTop: false
    // Should hitting the close button hide or destroy the window?
    property bool destroyOnCloseButton: true
    // Should hiding the window destroy it or just hide it?
    property bool destroyOnInvisible: false
    // FIXME support for pinned / unpinned pending full design
    // property bool pinnable: false
    // property bool pinned: false
    property bool resizable: false
    property bool gradientsSupported: desktop.gradientsSupported

    property vector2d minSize: Qt.vector2d(100, 100)
    property vector2d maxSize: Qt.vector2d(1280, 800)

    // The content to place inside the window, determined by the client
    default property var content

    property var footer: Item { }  // Optional static footer at the bottom of the dialog.

    function setDefaultFocus() {}  // Default function; can be overridden by dialogs.

    property var rectifier: Timer {
        property bool executing: false;
        interval: 100
        repeat: false
        running: false

        onTriggered: {
            executing = true;
            x = Math.floor(x);
            y = Math.floor(y);
            executing = false;
        }

        function begin() {
            if (!executing) {
                restart();
            }
        }
    }
    onXChanged: rectifier.begin();
    onYChanged: rectifier.begin();

    // This mouse area serves to raise the window. To function, it must live
    // in the window and have a higher Z-order than the content, but follow
    // the position and size of frame decoration
    property var activator: MouseArea {
        width: frame.decoration.width
        height: frame.decoration.height
        x: frame.decoration.anchors.leftMargin
        y: frame.decoration.anchors.topMargin
        propagateComposedEvents: true
        acceptedButtons: Qt.AllButtons
        enabled: window.visible
        onPressed: {
            //console.log("Pressed on activator area");
            window.raise();
            mouse.accepted = false;
        }
    }

    // This mouse area serves to swallow mouse events while the mouse is over the window
    // to prevent things like mouse wheel events from reaching the application and changing
    // the camera if the user is scrolling through a list and gets to the end.
    property var swallower: MouseArea {
        width: frame.decoration.width
        height: frame.decoration.height
        x: frame.decoration.anchors.leftMargin
        y: frame.decoration.anchors.topMargin
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        enabled: window.visible
        onClicked: {}
        onDoubleClicked: {}
        onPressAndHold: {}
        onReleased: {}
        onWheel: {}
    }

    // Default to a standard frame.  Can be overriden to provide custom
    // frame styles, like a full desktop frame to simulate a modal window
    property var frame: DefaultFrame { }

    // Scrollable window content.
    property var pane: Item {
        property bool isScrolling: scrollView.height < scrollView.contentItem.height
        property int contentWidth: scrollView.width - (isScrolling ? 10 : 0)
        property int scrollHeight: scrollView.height

        anchors.fill: parent
        anchors.rightMargin: isScrolling ? 11 : 0

        Rectangle {
            id: contentBackground
            anchors.fill: parent
            anchors.rightMargin: parent.isScrolling ? 11 : 0
            color: hifi.colors.baseGray
            visible: modality != Qt.ApplicationModal
        }

        LinearGradient {
            visible: gradientsSupported && modality != Qt.ApplicationModal
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
            contentItem: content
            horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
            verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
            anchors.fill: parent
            anchors.rightMargin: parent.isScrolling ? 1 : 0
            anchors.bottomMargin: footer.height > 0 ? footerPane.height : 0

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

        Rectangle {
            // Optional non-scrolling footer.
            id: footerPane
            anchors {
                left: parent.left
                bottom: parent.bottom
            }
            width: parent.contentWidth
            height: footer.height + 2 * hifi.dimensions.contentSpacing.y
            color: hifi.colors.baseGray
            visible: footer.height > 0

            Item {
                // Horizontal rule.
                anchors.fill: parent

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
                anchors.fill: parent
                anchors.topMargin: 3  // Horizontal rule.
                children: [ footer ]
            }
        }
    }

    children: [ swallower, frame, pane, activator ]

    Component.onCompleted: { raise(); setDefaultFocus(); }
    Component.onDestruction: windowDestroyed();
    onParentChanged: raise();

    onVisibleChanged: {
        if (!visible && destroyOnInvisible) {
            destroy();
            return;
        }
        if (visible) {
            raise();
        }
        enabled = visible
    }

    function raise() {
        if (visible && parent) {
            desktop.raise(window)
        }
    }

    function pin() {
//        pinned = ! pinned
    }

    // our close function performs the same way as the OffscreenUI class:
    // don't do anything but manipulate the targetVisible flag and let the other
    // mechanisms decide if the window should be destroyed after the close
    // animation completes
    // FIXME using this close function messes up the visibility signals received by the
    // type and it's derived types
//    function close() {
//        console.log("Closing " + window)
//        if (destroyOnCloseButton) {
//            destroyOnInvisible = true
//        }
//        visible = false;
//    }

    function framedRect() {
        if (!frame || !frame.decoration) {
            return Qt.rect(0, 0, window.width, window.height)
        }
        return Qt.rect(frame.decoration.anchors.leftMargin, frame.decoration.anchors.topMargin,
                       window.width - frame.decoration.anchors.leftMargin - frame.decoration.anchors.rightMargin,
                       window.height - frame.decoration.anchors.topMargin - frame.decoration.anchors.bottomMargin)
    }

    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_Control:
            case Qt.Key_Shift:
            case Qt.Key_Meta:
            case Qt.Key_Alt:
                break;

            case Qt.Key_W:
                if (window.closable && (event.modifiers === Qt.ControlModifier)) {
                    visible = false
                    event.accepted = true
                }
                // fall through

            default:
                // Consume unmodified keyboard entries while the window is focused, to prevent them
                // from propagating to the application
                if (event.modifiers === Qt.NoModifier) {
                    event.accepted = true;
                }
                break;
        }
    }
}
