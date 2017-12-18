import QtQuick 2.3
import QtQuick.Controls 1.2
import QtGraphicalEffects 1.0

import "."

Overlay {
    id: root

    Image {
        id: image
        property bool scaleFix: true
        property real xStart: 0
        property real yStart: 0
        property real xSize: 0
        property real ySize: 0
        property real imageScale: 1.0
        property var resizer: Timer {
            interval: 50
            repeat: false
            running: false
            onTriggered: {
                if (image.xSize === 0) {
                    image.xSize = image.sourceSize.width - image.xStart;
                }
                if (image.ySize === 0) {
                    image.ySize = image.sourceSize.height - image.yStart;
                }

                image.anchors.leftMargin = -image.xStart * root.width / image.xSize;
                image.anchors.topMargin = -image.yStart * root.height / image.ySize;
                image.anchors.rightMargin = (image.xStart + image.xSize - image.sourceSize.width) * root.width / image.xSize;
                image.anchors.bottomMargin = (image.yStart + image.ySize - image.sourceSize.height) * root.height / image.ySize;
            }
        }

        onSourceSizeChanged: {
            if (sourceSize.width !== 0 && sourceSize.height !== 0 && progress === 1.0 && scaleFix) {
                scaleFix = false;
                resizer.start();
            }
        }

        anchors.fill: parent
    }

    ColorOverlay {
        id: color
        anchors.fill: image
        source: image
    }

    function updateSubImage(subImage) {
        var keys = Object.keys(subImage);
        for (var i = 0; i < keys.length; ++i) {
            var key = keys[i];
            var value = subImage[key];
            switch (key) {
                case "x": image.xStart = value; break;
                case "y": image.yStart = value; break;
                case "width": image.xSize = value; break;
                case "height": image.ySize = value; break;
            }
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
                case "alpha": root.opacity = value; break;
                case "imageURL": image.source = value; break;
                case "subImage": updateSubImage(value); break;
                case "color": color.color = Qt.rgba(value.red / 255, value.green / 255, value.blue / 255, root.opacity); break;
                case "bounds": break; // The bounds property is handled in C++.
                default: console.log("OVERLAY Unhandled image property " + key);
            }
        }
    }
}

