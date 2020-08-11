import QtQuick 2.5
import QtQuick.Controls 1.4
//import Hifi 1.0 as Hifi

Rectangle {
    id: root
    property var window

    Binding { target: root; property:'window'; value: parent.parent; when: Boolean(parent.parent) }

    Binding { target: window; property: 'shown'; value: false; when: Boolean(window) }
    Component.onDestruction: thing && thing.destroy()

    property var hist : -1;
    property var history : [];

    signal sendToScript(var message);
    color: "#00000000"
    property alias thing: thing

    function sendMessage(text){
        sendToScript(text);
    }

    function fromScript(message) {
        console.log("fromScript "+message);
        var data = {failed:true};
        try{
            data = JSON.parse(message);
        } catch(e){
            //
        }
        if (!data.failed) {
            if (data.cmd) {
                JSConsole.executeCommand(data.msg);
            }
            console.log(data.visible);
            if (data.visible) {
                thing.visible = true;
                textArea.focus = true;
            } else if (!data.visible) {
                thing.visible = false;
                textArea.focus = false;
            }
            if (data.history) {
                history = data.history;
            }
        }
    }

    Rectangle {
        id: thing
        parent: desktop
        x: 0
        y: parent.height - height
        width: parent.width
        height: 64
        color: "#00000000"
        z: 99
        visible:true

        TextArea {
            id: textArea
            x: 64
            width: parent.width-64
            height: parent.height
            text:""
            textColor: "#000000"
            clip: false
            font.pointSize: 20

            function _onEnterPressed(event) {
                sendMessage(JSON.stringify({type:"MSG", message:text, event:event}));
                history.unshift(text);
                text = "";
                hist = -1;
            }

            function upPressed(event) {
                hist++;
                if (hist > history.length-1) {
                    hist = history.length-1;
                }
                text = history[hist];
            }
            function downPressed(event) {
                hist--;
                if (hist < -1) {
                    hist = -1;
                }
                if (hist === -1) {
                    text = "";
                 } else{
                    text = history[hist];
                 }
            }

            Keys.onReturnPressed: { _onEnterPressed(event); }
            Keys.onEnterPressed: { _onEnterPressed(event); }
            Keys.onUpPressed: { upPressed(event); }
            Keys.onDownPressed: { downPressed(event); }
        }

        MouseArea {
            anchors.rightMargin: -1
            anchors.bottomMargin: 0
            anchors.topMargin: 0
            anchors.leftMargin: 63
            anchors.fill: parent
            propagateComposedEvents: false
            acceptedButtons: Qt.AllButtons
            enabled: false
            onPressed: {
                thing.forceActiveFocus();
                mouse.accepted = true;
            }
        }

        Button {
            id: button
            x: 0
            y: 0
            width: 64
            height: 64
            text: qsTr("Button")
            clip: false
            opacity: 1
            iconSource: ""
            visible: true
            
            Image {
                id: image
                x: 0
                y: 0
                width: 64
                height: 64
                visible: true
                fillMode: Image.PreserveAspectFit
                source: "chat.png"
            }
        }

    }

    Connections {
        target: button
        onClicked:
            sendMessage(JSON.stringify({type:"CMD",cmd:"Clicked"}));
    }

}


/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
