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
import "./test_simulation_scene.js" as Sim


Rectangle {
    HifiConstants { id: hifi;}
    id: _test;   

    width: parent ? parent.width : 400
    height: parent ? parent.height : 600
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;

    property var scene : []

    function clearScene() {
        for (var i = 0; i < _test.scene.length; i++) {
            Entities.deleteEntity(_test.scene[i]);
        }
    }
    Component.onCompleted: {
        _test.scene = Sim.setupScene();

    }  
    Component.onDestruction: {
        clearScene()
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
        Separator {}

    }
}
