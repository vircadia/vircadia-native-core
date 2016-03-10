import QtQuick 2.3
import QtQuick.Controls 1.2

import "."

Overlay {
    id: root

    Rectangle {
        id: rectangle
        anchors.fill: parent
        color: "black"
    }

    function updatePropertiesFromScript(properties) {
        var keys = Object.keys(properties);
        for (var i = 0; i < keys.length; ++i) {
            var key = keys[i];
            var value = properties[key];
            switch (key) {
                case "height": root.height = value; break;
                case "width": root.width = value; break;
                case "x": root.x = value; break;
                case "y": root.y = value; break;
                case "visible": root.visible = value; break;
                case "alpha": rectangle.color.a = value; break;
                case "color": rectangle.color = Qt.rgba(value.red / 255, value.green / 255, value.blue / 255, rectangle.color.a); break;
                case "borderAlpha": rectangle.border.color.a = value; break;
                case "borderColor": rectangle.border.color = Qt.rgba(value.red / 255, value.green / 255, value.blue / 255, rectangle.border.color.a); break;
                case "borderWidth": rectangle.border.width = value; break;
                case "radius": rectangle.radius = value; break;
                default: console.warn("OVERLAY Unhandled rectangle property " + key);
            }
        }
    }
}

