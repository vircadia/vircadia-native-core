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
import "../lib/jet/qml" as Jet


Rectangle {
    HifiConstants { id: hifi;}
    id: _workload;   

    width: parent ? parent.width : 400
    height: parent ? parent.height : 600
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;

    function broadcastCreateScene() {
        sendToScript({method: "createScene", params: { count:2 }}); 
    }

    function broadcastClearScene() {
        sendToScript({method: "clearScene", params: { count:2 }}); 
    }

    function broadcastChangeResolution(value) {
        sendToScript({method: "changeResolution", params: { count:value }});         
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

        HifiControls.Label {
            text: "Workload"       
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right 
            HifiControls.CheckBox {
                boxSize: 20
                text: "Freeze Views"
                checked: Workload.getConfig("setupViews")["freezeViews"]
                onCheckedChanged: { Workload.getConfig("SpaceToRender")["freezeViews"] = checked, Workload.getConfig("setupViews")["freezeViews"] = checked; }
            }
            HifiControls.CheckBox {
                boxSize: 20
                text: "Use Avatar View"
                checked: Workload.getConfig("setupViews")["useAvatarView"]
                onCheckedChanged: { Workload.getConfig("setupViews")["useAvatarView"] = checked; }
            }
            HifiControls.CheckBox {
                boxSize: 20
                text: "force View Horizontal"
                checked: Workload.getConfig("setupViews")["forceViewHorizontal"]
                onCheckedChanged: { Workload.getConfig("setupViews")["forceViewHorizontal"] = checked; }
            }
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right 
            Column {
                anchors.left: parent.left
                anchors.right: parent.horizontalCenter 
                HifiControls.Label {
                    text: "Back [m]"       
                    anchors.horizontalCenter: parent.horizontalCenter 
                }
                Repeater {
                    model: [ 
                        "R1:r1Back:50.0:0.0",
                        "R2:r2Back:50.0:0.0",
                        "R3:r3Back:50.0:0.0"
                    ]
                    ConfigSlider {
                        label: qsTr(modelData.split(":")[0])
                        config:  Workload.getConfig("setupViews")
                        property: modelData.split(":")[1]
                        max: modelData.split(":")[2]
                        min: modelData.split(":")[3]
                        integral: true

                        labelAreaWidthScale: 0.4
                        anchors.left: parent.left
                        anchors.right: parent.right 
                    }
                }
            }
            Column {
                anchors.left: parent.horizontalCenter
                anchors.right: parent.right 
                HifiControls.Label {
                    text: "Front [m]"       
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Repeater {
                    model: [ 
                        "r1Front:300:1.0",
                        "r2Front:300:1.0",
                        "r3Front:300:1.0"
                    ]
                    ConfigSlider {
                        showLabel: false
                        config:  Workload.getConfig("setupViews")
                        property: modelData.split(":")[0]
                        max: modelData.split(":")[1]
                        min: modelData.split(":")[2]
                        integral: true

                        labelAreaWidthScale: 0.3
                        anchors.left: parent.left
                        anchors.right: parent.right 
                    }
                }
            }
        }
        Separator {}
        HifiControls.Label {
            text: "Display"       
        }
        HifiControls.CheckBox {
            boxSize: 20
            text: "Show Proxies"
            checked: Workload.getConfig("SpaceToRender")["showProxies"]
            onCheckedChanged: { Workload.getConfig("SpaceToRender")["showProxies"] = checked }
        }
        HifiControls.CheckBox {
            boxSize: 20
            text: "Show Views"
            checked: Workload.getConfig("SpaceToRender")["showViews"]
            onCheckedChanged: { Workload.getConfig("SpaceToRender")["showViews"] = checked }
        }
        Separator {}
         HifiControls.Label {
            text: "Test"       
        }
        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right 
            HifiControls.Button {
                text: "create scene"
                onClicked: {
                    print("pressed")
                    _workload.broadcastCreateScene()
                }
            }
            HifiControls.Button {
                text: "clear scene"
                onClicked: {
                    print("pressed")
                    _workload.broadcastClearScene()
                }
            }
        }
        HifiControls.Slider {
            id: resolution
            stepSize: 1.0
            anchors.left: parent.left
            anchors.right: parent.right 
            anchors.rightMargin: 0
            anchors.top: root.top
            anchors.topMargin: 0
            minimumValue: 1
            maximumValue: 10

            onValueChanged: { _workload.broadcastChangeResolution(value) }
        }

        Separator {}
        
        /*Jet.TaskList {
            rootConfig: Workload
            anchors.left: parent.left
            anchors.right: parent.right 
        
            height: 300
        }*/
    }
}
