import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0

import "."
import "../styles"

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
    implicitHeight: content.height
    implicitWidth: content.width
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
    property vector2d minSize: Qt.vector2d(100, 100)
    property vector2d maxSize: Qt.vector2d(1280, 720)

    // The content to place inside the window, determined by the client
    default property var content

    // This mouse area serves to raise the window. To function, it must live
    // in the window and have a higher Z-order than the content, but follow
    // the position and size of frame decoration
    property var activator: MouseArea {
        width: frame.decoration.width
        height: frame.decoration.height
        x: frame.decoration.anchors.margins
        y: frame.decoration.anchors.topMargin
        propagateComposedEvents: true
        hoverEnabled: true
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
        x: frame.decoration.anchors.margins
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


    children: [ swallower, frame, content, activator ]

    Component.onCompleted: raise();
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
