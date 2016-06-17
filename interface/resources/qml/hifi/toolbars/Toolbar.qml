import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../windows"
import "."

Window {
    id: window
    frame: ToolFrame { }
    hideBackground: true
    resizable: false
    destroyOnCloseButton: false
    destroyOnHidden: false
    closable: false
    shown: true
    pinned: true
    width: content.width
    height: content.height
    visible: true
    // Disable this window from being able to call 'desktop.raise() and desktop.showDesktop'
    activator: Item {}
    property bool horizontal: true
    property real buttonSize: 50;
    property var buttons: []
    property var container: horizontal ? row : column

    Settings {
        category: "toolbar/" + window.objectName
        property alias x: window.x
        property alias y: window.y
    }

    onHorizontalChanged: {
        var oldParent = horizontal ? column : row;
        var newParent = horizontal ? row : column;
        var move = [];

        var i;
        for (i in oldParent.children) {
            var child = oldParent.children[i];
            if (child.spacer) {
                continue;
            }
            move.push(oldParent.children[i]);
        }
        for (i in move) {
            move[i].parent = newParent;
            if (horizontal) {
                move[i].y = 0
            } else {
                move[i].x = 0
            }
        }
        fixSpacers();
    }

    Item {
        id: content
        implicitHeight: horizontal ? row.height : column.height
        implicitWidth: horizontal ? row.width : column.width
        Row {
            id: row
            spacing: 6
            visible: window.horizontal
            Rectangle{ readonly property bool spacer: true; id: rowSpacer1; width: 1; height: row.height }
            Rectangle{ readonly property bool spacer: true; id: rowSpacer2; width: 1; height: row.height }
            Rectangle{ readonly property bool spacer: true; id: rowSpacer3; width: 1; height: row.height }
            Rectangle{ readonly property bool spacer: true; id: rowSpacer4; width: 1; height: row.height }
        }

        Column {
            id: column
            spacing: 6
            visible: !window.horizontal
            Rectangle{ readonly property bool spacer: true; id: colSpacer1; width: column.width; height: 1 }
            Rectangle{ readonly property bool spacer: true; id: colSpacer2; width: column.width; height: 1 }
            Rectangle{ readonly property bool spacer: true; id: colSpacer3; width: column.width; height: 1 }
            Rectangle{ readonly property bool spacer: true; id: colSpacer4; width: column.width; height: 1 }
        }

        Component { id: toolbarButtonBuilder; ToolbarButton { } }
    }

    function addButton(properties) {
        properties = properties || {}

        // If a name is specified, then check if there's an existing button with that name
        // and return it if so.  This will allow multiple clients to listen to a single button,
        // and allow scripts to be idempotent so they don't duplicate buttons if they're reloaded
        if (properties.objectName) {
            for (var i in buttons) {
                var child = buttons[i];
                if (child.objectName === properties.objectName) {
                    return child;
                }
            }
        }

        properties.toolbar = this;
        var result = toolbarButtonBuilder.createObject(container, properties);
        buttons.push(result);
        fixSpacers();
        return result;
    }

    function fixSpacers() {
        colSpacer3.parent = null
        colSpacer4.parent = null
        rowSpacer3.parent = null
        rowSpacer4.parent = null
        colSpacer3.parent = column
        colSpacer4.parent = column
        rowSpacer3.parent = row
        rowSpacer4.parent = row
    }
}
