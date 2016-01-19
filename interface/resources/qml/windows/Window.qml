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

    property int modality: Qt.NonModal

    readonly property bool topLevelWindow: true
    property string title
    // Should the window be closable control?
    property bool closable: true
    // Should the window try to remain on top of other windows?
    property bool alwaysOnTop: false
    // Should hitting the close button hide or destroy the window?
    property bool destroyOnCloseButton: true
    // FIXME support for pinned / unpinned pending full design
    // property bool pinnable: false
    // property bool pinned: false
    property bool resizable: false
    property vector2d minSize: Qt.vector2d(100, 100)
    property vector2d maxSize: Qt.vector2d(1280, 720)
    enabled: visible

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
        onPressed: {
            //console.log("Pressed on activator area");
            window.raise();
            mouse.accepted = false;
        }
        // Debugging
//        onEntered: console.log("activator entered")
//        onExited: console.log("activator exited")
//        onContainsMouseChanged: console.log("Activator contains mouse " + containsMouse)
//        onPositionChanged: console.log("Activator mouse position " + mouse.x + " x " + mouse.y)
//        Rectangle { anchors.fill:parent; color: "#7f00ff00" }
    }

    signal windowDestroyed();

    // Default to a standard frame.  Can be overriden to provide custom
    // frame styles, like a full desktop frame to simulate a modal window
    property var frame;

    Component {
        id: defaultFrameBuilder;
        DefaultFrame { anchors.fill: parent }
    }

    Component {
        id: modalFrameBuilder;
        ModalFrame { anchors.fill: parent }
    }

    Component.onCompleted: {
        if (!frame) {
            if (modality === Qt.NonModal) {
                frame = defaultFrameBuilder.createObject(window);
            } else {
                frame = modalFrameBuilder.createObject(window);
            }
        }
        raise();
    }

    children: [ frame, content, activator ]

    Component.onDestruction: {
        content.destroy();
        console.log("Destroyed " + window);
        windowDestroyed();
    }
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
            Desktop.raise(window)
            if (!focus) {
                focus = true;
            }
        }
    }

    function pin() {
//        pinned = ! pinned
    }

    // our close function performs the same way as the OffscreenUI class:
    // don't do anything but manipulate the targetVisible flag and let the other
    // mechanisms decide if the window should be destroyed after the close
    // animation completes
    function close() {
        console.log("Closing " + window)
        if (destroyOnCloseButton) {
            destroyOnInvisible = true
        }
        visible = false;
    }

    function clamp(value, min, max) {
        return Math.min(Math.max(value, min), max);
    }

    function clampVector(value, min, max) {
        return Qt.vector2d(
                    clamp(value.x, min.x, max.x),
                    clamp(value.y, min.y, max.y))
    }

    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_W:
                if (window.closable && (event.modifiers === Qt.ControlModifier)) {
                    visible = false
                    event.accepted = true
                }
                break
        }
    }
}
