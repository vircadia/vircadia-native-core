import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls
import ".."

Item {
    id: modesbar
    y:60
	Rectangle {
        anchors.fill : parent
 		color: "transparent"
        Flow {
            id: flowMain
            spacing: 0
            flow: Flow.TopToBottom
            layoutDirection: Flow.TopToBottom
            anchors.fill: parent
            anchors.margins: 4
        }
	}

    Component.onCompleted: {
        width = 330;
        height = 330;
        x=Window.innerWidth - width;
    }
    
    function addButton(properties) {
        var component = Qt.createComponent("button.qml");
        console.log("load button");
        if (component.status == Component.Ready) {
            console.log("load button 2");
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
            case "allButtonsShown":
                modesbar.height = flowMain.children.length * 100 + 10;
            break;
            case "inactiveButtonsHidden":
                modesbar.height = 100 + 10;
            break;
            default:
            break;
        }
    }

}
