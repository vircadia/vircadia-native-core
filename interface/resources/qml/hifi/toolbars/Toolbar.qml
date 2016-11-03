import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../windows"
import "."

Window {
    id: window
    frame: ToolFrame {
        horizontalSpacers: horizontal
        verticalSpacers: !horizontal
    }
    hideBackground: true
    resizable: false
    destroyOnCloseButton: false
    destroyOnHidden: false
    closable: false
    shown: true
    width: content.width
    height: content.height
    // Disable this window from being able to call 'desktop.raise() and desktop.showDesktop'
    activator: Item {}
    property bool horizontal: true
    property real buttonSize: 50;
    property var buttons: []
    property var container: horizontal ? row : column
	property bool constrainToolbarToCenterX: false
            
    Settings {
        category: "toolbar/" + window.objectName
        property alias x: window.x
        property alias y: window.y
		property alias constrainToolbarToCenterX: window.constrainToolbarToCenterX
    }

    onHorizontalChanged: {
        var newParent = horizontal ? row : column;
        for (var i in buttons) {
            var child = buttons[i];
            child.parent = newParent;
            if (horizontal) {
                child.y = 0
            } else {
                child.x = 0
            }
        }
    }

    Item {
        id: content
        implicitHeight: horizontal ? row.height : column.height
        implicitWidth: horizontal ? row.width : column.width

        Row {
            id: row
            spacing: 6
        }

        Column {
            id: column
            spacing: 6
        }

        Component { id: toolbarButtonBuilder; ToolbarButton { } }

        Connections {
            target: desktop
            onPinnedChanged: {
                if (!window.pinned) {
                    return;
                }
                var newPinned = desktop.pinned;
                for (var i in buttons) {
                    var child = buttons[i];
                    if (desktop.pinned) {
                        if (!child.pinned) {
                            child.visible = false;
                        }
                    } else {
                        child.visible = true;
                    }
                }
            }
        }
    }


    function findButtonIndex(name) {
        if (!name) {
            return -1;
        }

        for (var i in buttons) {
            var child = buttons[i];
            if (child.objectName === name) {
                return i;
            }
        }
        return -1;
    }

    function findButton(name) {
        var index = findButtonIndex(name);
        if (index < 0) {
            return;
        }
        return buttons[index];
    }

    function addButton(properties) {
        properties = properties || {}

        // If a name is specified, then check if there's an existing button with that name
        // and return it if so.  This will allow multiple clients to listen to a single button,
        // and allow scripts to be idempotent so they don't duplicate buttons if they're reloaded
        var result = findButton(properties.objectName);
        if (result) {
            return result;
        }
        properties.toolbar = this;
        properties.opacity = 0;
        result = toolbarButtonBuilder.createObject(container, properties);
        buttons.push(result);
        result.opacity = 1;
        updatePinned();
        return result;
    }

    function removeButton(name) {
        var index = findButtonIndex(name);
        if (index < -1) {
            console.warn("Tried to remove non-existent button " + name);
            return;
        }
        buttons[index].destroy();
        buttons.splice(index, 1);
        updatePinned();
    }

    function updatePinned() {
        var newPinned = false;
        for (var i in buttons) {
            var child = buttons[i];
            if (child.pinned) {
                newPinned = true;
                break;
            }
        }
        pinned = newPinned;
    }
}
