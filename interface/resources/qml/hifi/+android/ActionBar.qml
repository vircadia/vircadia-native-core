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
    id: actionBar
    x:0
    y:0
    width: 300
    height: 300
    z: -1

    signal sendToScript(var message);
    signal windowClosed();
    
    property bool shown: true

    onShownChanged: {
        actionBar.visible = shown;
    }

	Rectangle {
        anchors.fill : parent
        color: "transparent"
        Flow {
            id: flowMain
            spacing: 10
            flow: Flow.TopToBottom
            layoutDirection: Flow.TopToBottom
            anchors.fill: parent
            anchors.margins: 4
        }
    }

    Component.onCompleted: {
        // put on bottom
        x = 7;
        y = 7;
        width = 300;
        height = 300;
    }
    
    function addButton(properties) {
        var component = Qt.createComponent("button.qml");
        if (component.status == Component.Ready) {
            var button = component.createObject(flowMain);
            // copy all properites to button
            var keys = Object.keys(properties).forEach(function (key) {
                button[key] = properties[key];
            });
            return button;
        } else if( component.status == Component.Error) {
            console.log("Load button errors " + component.errorString());
        }
    }

    function urlHelper(src) {
            if (src.match(/\bhttp/)) {
                return src;
            } else {
                return "../../../" + src;
            }
        }

}
