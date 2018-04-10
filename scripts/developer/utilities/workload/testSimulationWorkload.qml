//
//  _workload.qml
//
//  Created by Sam Gateau on 3/1/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls
import "../render/configSlider"
//import "./test_simulation_scene.js" as Sim


Rectangle {
    HifiConstants { id: hifi;}
    id: _test;   

    width: parent ? parent.width : 400
    height: parent ? parent.height : 600
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;

    Component.onCompleted: {
    }  
    Component.onDestruction: {
    }

    function broadcastCreateScene() {
        sendToScript({method: "createScene", params: { count:2 }}); 
    }

    function broadcastClearScene() {
        sendToScript({method: "clearScene", params: { count:2 }}); 
    }

    function fromScript(message) {
        switch (message.method) {
        }
    }

    Column {
        spacing: 5
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  
        //padding: hifi.dimensions.contentMargin.x

        
        
        Separator {}
        HifiControls.Label {
            text: "Display"       
        }

        HifiControls.Button {
                    text: "create scene"
                    onClicked: {
                        print("pressed")
                        _test.broadcastCreateScene()
                    }
                }

        Separator {}

    }
}
