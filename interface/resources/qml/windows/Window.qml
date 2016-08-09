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
    signal windowClosed();
    signal windowDestroyed();
    signal mouseEntered();
    signal mouseExited();

    //
    // Native properties
    //

    // The Window size is the size of the content, while the frame
    // decorations can extend outside it.
    implicitHeight: content ? content.height : 0
    implicitWidth: content ? content.width : 0
    x: desktop.invalid_position; y: desktop.invalid_position;
    children: [ swallower, frame, content, activator ]

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
        width: frame.decoration ? frame.decoration.width : window.width
        height: frame.decoration ? frame.decoration.height : window.height
        x: frame.decoration ? frame.decoration.anchors.leftMargin : 0
        y: frame.decoration ? frame.decoration.anchors.topMargin : 0
        propagateComposedEvents: true
        acceptedButtons: Qt.AllButtons
        enabled: window.visible
        onPressed: {
            window.raise();
            mouse.accepted = false;
        }
    }

    // This mouse area serves to swallow mouse events while the mouse is over the window
    // to prevent things like mouse wheel events from reaching the application and changing
    // the camera if the user is scrolling through a list and gets to the end.
    property var swallower: MouseArea {
        width: frame.decoration ? frame.decoration.width : window.width
        height: frame.decoration ? frame.decoration.height : window.height
        x: frame.decoration ? frame.decoration.anchors.leftMargin : 0
        y: frame.decoration ? frame.decoration.anchors.topMargin : 0
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
    property var frame: DefaultFrame {
        //window: window
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
                if (force) {
                    window.raise();
                }
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

    onMouseEntered: console.log("Mouse entered " + window)
    onMouseExited: console.log("Mouse exited " + window)
}
