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
                case "color":
                case "textColor": textField.color = Qt.rgba(value.red, value.green, value.blue, textField.color.a); break;
                case "text": textField.text = value; break;
                case "backgroundAlpha":
                    if ("object" === typeof(value)) {
                        console.log("OVERLAY Unexpected object for alpha");
                        dumpObject(value)
                    } else {
                        background.color.a = value; break;
                    }
                    break
                case "backgroundColor": background.color = Qt.rgba(value.red, value.green, value.blue, background.color.a); break;
                case "font": if (typeof(value) === "Object") {
                        console.log("Font object");
                        dumpObject(value)
                    }
                    break;
                default:
                    console.log("OVERLAY text unhandled property " + key);
            }
        }
    }
}
