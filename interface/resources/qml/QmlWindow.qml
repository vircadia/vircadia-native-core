
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebChannel 1.0
import QtWebSockets 1.0
import "qrc:///qtwebchannel/qwebchannel.js" as WebChannel

import "controls"
import "styles"

VrDialog {
    id: root
    HifiConstants { id: hifi }
    title: "QmlWindow"
    resizable: true
    enabled: false
    focus: true
    property var channel;
    

    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight
    property alias source: pageLoader.source 

    /*
    WebSocket {
        id: socket
        url: "ws://localhost:51016";
        active: false

        // the following three properties/functions are required to align the QML WebSocket API with the HTML5 WebSocket API.
        property var send: function (arg) {
            sendTextMessage(arg);
        }

        onTextMessageReceived: {
            onmessage({data: message});
        }

        property var onmessage;

        onStatusChanged: {
            if (socket.status == WebSocket.Error) {
                console.error("Error: " + socket.errorString)
             } else if (socket.status == WebSocket.Closed) {
                 console.log("Socket closed");
             } else if (socket.status == WebSocket.Open) {
                 console.log("Connected")
                 //open the webchannel with the socket as transport
                 new WebChannel.QWebChannel(socket, function(ch) {
                    root.channel = ch;
                    var myUrl = root.source.toString().toLowerCase();
                    console.log(myUrl);
                    var bridge = root.channel.objects[myUrl];
                    console.log(bridge);
                });
             }
         }
    }
    */
    Keys.onPressed: {
        console.log("QmlWindow keypress")
    }
    
    Item {
        id: clientArea
        implicitHeight: 600
        implicitWidth: 800
        x: root.clientX
        y: root.clientY
        width: root.clientWidth
        height: root.clientHeight
        focus: true

        Loader { 
            id: pageLoader
            objectName: "Loader"
            anchors.fill: parent
            focus: true
            
            onLoaded: {
                console.log("Loaded content")
                //socket.active = true; //connect
                forceActiveFocus()
            }
            
            Keys.onPressed: {
                console.log("QmlWindow pageLoader keypress")
            }
        }
    } // item
} // dialog
