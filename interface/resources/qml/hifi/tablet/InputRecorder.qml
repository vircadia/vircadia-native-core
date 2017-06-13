//
//  Created by Dante Ruiz 2017/04/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import Hifi 1.0
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs

import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../windows"
import "../../dialogs"

Rectangle {
    id: inputRecorder
    HifiConstants { id: hifi }
    signal sendToScript(var message);
    color: hifi.colors.baseGray;
    property string path: ""
    property string dir: ""
    property var dialog: null;
    property bool recording: false;

    Component { id: fileDialog; TabletFileDialog { } }
    Row {
        id: topButtons
        width: parent.width
        height: 40
        spacing: 40
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 10
        }
        
        HifiControls.Button {
            id: start
            text: "Start Recoring"
            color: hifi.buttons.black
            enabled: true
            onClicked: {
                if (inputRecorder.recording) {
                    sendToScript({method: "Stop"});
                    inputRecorder.recording = false;
                    start.text = "Start Recording";
                    selectedFile.text = "Current recording is not saved";
                } else {
                    sendToScript({method: "Start"});
                    inputRecorder.recording = true;
                    start.text = "Stop Recording";
                }
            }
        }

        HifiControls.Button {
            id: save
            text: "Save Recording"
            color: hifi.buttons.black
            enabled: true
            onClicked: {
                sendToScript({method: "Save"});
                selectedFile.text = "";
            }
        }

        HifiControls.Button {
            id: playBack
            anchors.right: browse.left
            anchors.top: selectedFile.bottom
            anchors.topMargin: 10
            
            text: "Play Recording"
            color: hifi.buttons.black
            enabled: true
            onClicked: {
                sendToScript({method: "playback"});
                HMD.closeTablet();
            }
        }
        
    }        
    
    HifiControls.VerticalSpacer {}    
    
    HifiControls.TextField {
        id: selectedFile
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topButtons.top
        anchors.topMargin: 40
            
        colorScheme: hifi.colorSchemes.dark
        readOnly: true
        
    }
        
   

    HifiControls.Button {
        id: browse
        anchors.right: parent.right
        anchors.top: selectedFile.bottom
        anchors.topMargin: 10
        
        text: "Load..."
        color: hifi.buttons.black
        enabled: true
        onClicked: {
            dialog = fileDialog.createObject(inputRecorder);
            dialog.caption = "InputRecorder";
            console.log(dialog.dir);
            dialog.dir = "file:///" + inputRecorder.dir;
            dialog.selectedFile.connect(getFileSelected);
        }
    }

    Column {
        id: notes
        anchors.centerIn: parent;
        spacing: 20

        Text {
            text: "All files are saved under the folder 'hifi-input-recording' in AppData directory";
            color: "white"
            font.pointSize: 10
        }

        Text {
            text: "To cancel a recording playback press Alt-B"
            color: "white"
            font.pointSize: 10
        }
    }
    
    function getFileSelected(file) {
        selectedFile.text = file;
        inputRecorder.path = file;
        sendToScript({method: "Load", params: {file: path }}); 
    }

    function fromScript(message) {
        switch (message.method) {
        case "update":
            updateButtonStatus(message.params);
            break;
        case "path":
            console.log(message.params);
            inputRecorder.dir = message.params;
            break;
        }
    }

    function updateButtonStatus(status) {
        inputRecorder.recording = status;

        if (inputRecorder.recording) {
            start.text = "Stop Recording";
        } else {
            start.text = "Start Recording";
        }
    }
}

