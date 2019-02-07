import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../controls" as HifiControls
import ".."

Item {
    id: modesbar
    y:5

    function relocateAndResize(newWindowWidth, newWindowHeight) {
        width = 300;
        height = 300;
        x = newWindowWidth - 565;
    }

    function onWindowGeometryChanged(rect) {
        relocateAndResize(rect.width, rect.height);
    }

    Component.onCompleted: {
        relocateAndResize(parent.width, parent.height);
        Window.geometryChanged.connect(onWindowGeometryChanged); // In devices with bars appearing at startup we should listen for this
    }

    Component.onDestruction: {
        Window.geometryChanged.disconnect(onWindowGeometryChanged);
    }
    
    function addButton(properties) {
        var component = Qt.createComponent("button.qml");
        console.log("load button");
        if (component.status == Component.Ready) {
            console.log("load button 2");
            var button = component.createObject(modesbar);
            // copy all properites to button
            var keys = Object.keys(properties).forEach(function (key) {
                button[key] = properties[key];
            });
            return button;
        } else if( component.status == Component.Error) {
            console.log("Load button errors " + component.errorString());
        }
    }

    function removeButton(name) {
    }

    function urlHelper(src) {
        if (src.match(/\bhttp/)) {
            return src;
        } else {
            return "../../../" + src;
        }
    }

    function fromScript(message) {
        switch (message.type) {
            case "switch":
                    // message.params.to
                    // still not needed 
                break;
            default:
                break;
        }
    }

}
