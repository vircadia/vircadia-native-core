import QtQuick 2.3
import QtQuick.Controls 1.2
import QtGraphicalEffects 1.0

import "."

Overlay {
    id: root

    Image {
        id: image
        property bool scaleFix: true;
        property real xOffset: 0
        property real yOffset: 0
        property real imageScale: 1.0
        property var resizer: Timer {
            interval: 50
            repeat: false
            running: false
            onTriggered: {
                var targetAspect = root.width / root.height;
                var sourceAspect = image.sourceSize.width / image.sourceSize.height;
                if (sourceAspect <= targetAspect) {
                    if (root.width === image.sourceSize.width) {
                        return;
                    }
                    image.imageScale = root.width / image.sourceSize.width;
                } else if (sourceAspect > targetAspect){
                    if (root.height === image.sourceSize.height) {
                        return;
                    }
                    image.imageScale = root.height / image.sourceSize.height;
                }
                image.sourceSize = Qt.size(image.sourceSize.width * image.imageScale, image.sourceSize.height * image.imageScale);
            }
        }
        x: -1 * xOffset * imageScale
        y: -1 * yOffset * imageScale

        onSourceSizeChanged: {
            if (sourceSize.width !== 0 && sourceSize.height !== 0 && progress === 1.0 && scaleFix) {
                scaleFix = false;
                resizer.start();
            }
        }
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
                case "x": image.xOffset = value; break;
                case "y": image.yOffset = value; break;
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
                default: console.log("OVERLAY Unhandled image property " + key);
            }
        }
    }
}

