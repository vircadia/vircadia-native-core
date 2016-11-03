import QtQuick 2.3
import QtQuick.Controls 1.2

import "."

Overlay {
    id: root
    clip: true
    Rectangle {
        id: background;
        anchors.fill: parent
        color: "#B2000000"

        Text {
            id: textField;
            anchors { fill: parent; bottomMargin: textField.anchors.topMargin; rightMargin: textField.anchors.leftMargin; }
            objectName: "textElement"
            color: "white"
            lineHeightMode: Text.FixedHeight
            font.family: "Helvetica"
            font.pixelSize: 18
            lineHeight: 18
        }
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
                case "alpha": textField.color.a = value; break;
                case "margin": textField.anchors.margins = value; break;
                case "leftMargin": textField.anchors.leftMargin = value; break;
                case "topMargin": textField.anchors.topMargin = value; break;
                case "color": // fall through
                case "textColor": textField.color = Qt.rgba(value.red / 255, value.green / 255, value.blue / 255, textField.color.a); break;
                case "text": textField.text = value; break;
                case "backgroundAlpha": background.color = Qt.rgba(background.color.r, background.color.g, background.color.b, value); break;
                case "backgroundColor": background.color = Qt.rgba(value.red / 255, value.green / 255, value.blue / 255, background.color.a); break;
                case "font": textField.font.pixelSize = value.size; break;
                case "lineHeight": textField.lineHeight = value; break;
                default: console.warn("OVERLAY text unhandled property " + key);
            }
        }
    }
}
