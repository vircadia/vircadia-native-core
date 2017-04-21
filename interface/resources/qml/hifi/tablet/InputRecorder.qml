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

Rectangle {
    id: inputRecorder
    property var eventBridge;
    HifiConstants { id: hifi }
    signal sendToScript(var message);
    color: hifi.colors.baseGray;


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
            text: "Start"
            color: hifi.buttons.blue
            enabled: true
        }

        HifiControls.Button {
            id: stop    
            text: "Stop"
            color: hifi.buttons.blue
            enabled: true
        }

        HifiControls.Button {
            id: save
            text: "Save"
            color: hifi.buttons.blue
            enabled: true
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
        id: load
        anchors.right: parent.right
        anchors.top: selectedFile.bottom
        anchors.topMargin: 10
        
        text: "Load"
        color: hifi.buttons.black
        enabled: true
    }

    HifiControls.Button {
        id: browse
        anchors.right: load.left
        anchors.top: selectedFile.bottom
        anchors.topMargin: 10
        
        text: "Browse"
        color: hifi.buttons.black
        enabled: true

        Trigger {
            id: browseTimer
            interval: 5
            repeat: false
            running: false
            
    }
}

