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

    //
    // Signals
    //
    signal windowDestroyed();

    //
    // Native properties
    //

    // The Window size is the size of the content, while the frame
    // decorations can extend outside it.
    implicitHeight: content ? content.height : 0
    implicitWidth: content ? content.width : 0
    x: desktop.invalid_position; y: desktop.invalid_position;
    children: [ swallower, frame, pane, activator ]

    //
    // Custom properties
    //

    property int modality: Qt.NonModal
    // Corresponds to the window shown / hidden state AS DISTINCT from window visibility.
    // Window visibility should NOT be used as a proxy for any other behavior.
    property bool shown: true
    // FIXME workaround to deal with the face that some visual items are defined here,
    // when they should be moved to a frame derived type
    property bool hideBackground: false
    visible: shown
    enabled: visible
    readonly property bool topLevelWindow: true
    property string title
    // Should the window be closable control?
    property bool closable: true
    // Should the window try to remain on top of other windows?
    property bool alwaysOnTop: false
    // Should hitting the close button hide or destroy the window?
    property bool destroyOnCloseButton: true
    // Should hiding the window destroy it or just hide it?
    property bool destroyOnHidden: false
    property bool pinnable: true
    property bool pinned: false
    property bool resizable: false
    property bool gradientsSupported: desktop.gradientsSupported
    property int colorScheme: hifi.colorSchemes.dark

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
    // FIXME this should not define any visual content in this type.  The base window
    // type should only consist of logic sized areas, with nothing drawn (although the
    // default value for the frame property does include visual decorations)
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
            visible: !window.hideBackground && modality != Qt.ApplicationModal
        }


        LinearGradient {
            visible: !window.hideBackground && gradientsSupported && modality != Qt.ApplicationModal
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
            height: footer.height + 2 * hifi.dimensions.contentSpacing.y + 3
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

    //
    // Handlers
    //
    Component.onCompleted: {
        window.parentChanged.connect(raise);
        setDefaultFocus();
        d.centerOrReposition();
        d.updateVisibility(shown);
    }
    Component.onDestruction: {
        window.parentChanged.disconnect(raise);  // Prevent warning on shutdown
        windowDestroyed();
    }

    onXChanged: rectifier.begin();
    onYChanged: rectifier.begin();

    onShownChanged: d.updateVisibility(shown)

    onVisibleChanged: {
        enabled = visible
        if (visible && parent) {
            d.centerOrReposition();
        }
    }

    QtObject {
        id: d

        readonly property alias pinned: window.pinned
        readonly property alias shown: window.shown
        readonly property alias modality: window.modality;

        function getTargetVisibility() {
            if (!window.shown) {
                return false;
            }

            if (modality !== Qt.NonModal) {
                return true;
            }

            if (pinned) {
                return true;
            }

            if (desktop && !desktop.pinned) {
                return true;
            }

            return false;
        }

        // The force flag causes all windows to fade back in, because a window was shown
        readonly property alias visible: window.visible
        function updateVisibility(force) {
            if (force && !pinned && desktop.pinned) {
                // Change the pinned state (which in turn will call us again)
                desktop.pinned = false;
                return;
            }

            var targetVisibility = getTargetVisibility();
            if (targetVisibility === visible) {
                return;
            }

            if (targetVisibility) {
                fadeIn(function() {
                    if (force) {
                        window.raise();
                    }
                });
            } else {
                fadeOut(function() {
                    if (!window.shown && window.destroyOnHidden) {
                        window.destroy();
                    }
                });
            }
        }

        function centerOrReposition() {
            if (x == desktop.invalid_position && y == desktop.invalid_position) {
                desktop.centerOnVisible(window);
            } else {
                desktop.repositionOnVisible(window);
            }
        }

    }

    // When the desktop pinned state changes, automatically handle the current windows
    Connections { target: desktop;  onPinnedChanged: d.updateVisibility() }


    function raise() {
        if (visible && parent) {
            desktop.raise(window)
        }
    }

    function setPinned() {
        pinned = !pinned
    }

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
                    shown = false
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
