import QtQuick 2.5
import QtQuick.Controls 1.4 as Controls

import "../../qml/controls-uit"
import "../../qml/styles-uit"

Item {
    id: menu
    HifiConstants { id: hifi  }
    signal done()
    implicitHeight: column.height
    implicitWidth: column.width

    Rectangle {
        id: background
        anchors {
            fill: parent
            margins: -16
        }
        radius: hifi.dimensions.borderRadius
        border.width: hifi.dimensions.borderWidth
        border.color: hifi.colors.lightGrayText80
        color: hifi.colors.faintGray80
    }

    MouseArea {
        id: closer
        width: 8192
        height: 8192
        x: -4096
        y: -4096
        propagateComposedEvents: true
        acceptedButtons: "AllButtons"
        onClicked: {
            menu.visible = false;
            menu.done();
            mouse.accepted = false;
        }
    }

    Column {
        id: column
    }

    function popup() {
        var position = Reticle.position;
        var localPosition = menu.parent.mapFromItem(desktop, position.x, position.y);
        x = localPosition.x
        y = localPosition.y
        console.log("Popup at " + x + " x " + y)
        var moveChildren = [];
        for (var i = 0; i < children.length; ++i) {
            var child = children[i];
            if (child.objectName !== "MenuItem") {
                continue;
            }
            moveChildren.push(child);
        }

        for (i = 0; i < moveChildren.length; ++i) {
            child = moveChildren[i];
            child.parent = column;
            child.menu = menu
        }
    }

}

