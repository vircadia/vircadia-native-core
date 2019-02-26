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

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import "../render/configSlider"
import "../lib/jet/qml" as Jet
import "../lib/plotperf"


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

    function broadcastChangeSize(value) {
        sendToScript({method: "changeSize", params: { count:value }});         
    }

    function broadcastChangeResolution(value) {
        sendToScript({method: "changeResolution", params: { count:value }});         
    }

    function broadcastBumpUpFloor(value) {
        sendToScript({method: "bumpUpFloor", params: { count:0 }});         
    }

    function fromScript(message) {
        switch (message.method) {
        case "gridSize":
            print("assigned value! " + message.params.v)
            gridSizeLabel.text = ("Grid size [m] = " + message.params.v)
            gridSize.setValue(message.params.v)
            break;
        case "resolution":
            print("assigned value! " + message.params.v)
            resolution.setValue(message.params.v)
            break;           
        case "objectCount":
            print("assigned objectCount! " + message.params.v)
            objectCount.text = ("Num objects = " + message.params.v)
            break;    
        }
    }
  
    Column {
        id: stats
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
            HifiControls.CheckBox {
                boxSize: 20
                text: "Simulate Secondary"
                checked: Workload.getConfig("setupViews")["simulateSecondaryCamera"]
                onCheckedChanged: { Workload.getConfig("setupViews")["simulateSecondaryCamera"] = checked; }
            }
        }

        Separator {}
        HifiControls.CheckBox {
            boxSize: 20
            text: "Regulate View Ranges"
            checked: Workload.getConfig("controlViews")["regulateViewRanges"]
            onCheckedChanged: { Workload.getConfig("controlViews")["regulateViewRanges"] = checked; }
        }

        RowLayout {
            visible: !Workload.getConfig("controlViews")["regulateViewRanges"]
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
                        "R1:r1Back:250.0:0.0",
                        "R2:r2Back:250.0:0.0",
                        "R3:r3Back:250.0:0.0"
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
                        "r1Front:16000:1.0",
                        "r2Front:16000:1.0",
                        "r3Front:16000:1.0"
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
        /*RowLayout {
            visible: Workload.getConfig("controlViews")["regulateViewRanges"]
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
                        "R1:r1RangeBack:50.0:0.0",
                        "R2:r2RangeBack:50.0:0.0",
                        "R3:r3RangeBack:50.0:0.0"
                    ]
                    ConfigSlider {
                        label: qsTr(modelData.split(":")[0])
                        config:  Workload.getConfig("controlViews")
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
                        "r1RangeFront:300:1.0",
                        "r2RangeFront:300:1.0",
                        "r3RangeFront:300:1.0"
                    ]
                    ConfigSlider {
                        showLabel: false
                        config:  Workload.getConfig("controlViews")
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
        }*/
        property var controlViews: Workload.getConfig("controlViews")

        PlotPerf {
            title: "Timings"
            height: 100
            object: stats.controlViews
            valueScale: 1.0
            valueUnit: "ms"
            plots: [
                {
                    prop: "r2Timing",
                    label: "Physics + Collisions",
                    color: "#1AC567"
                },
                {
                    prop: "r3Timing",
                    label: "Kinematic + Update",
                    color: "#1A67C5"
                }
            ]
        }
      /*  PlotPerf {
            title: "Ranges"
            height: 100
            object: stats.controlViews
            valueScale: 1.0
            valueUnit: "m"
            plots: [
                {
                    prop: "r3RangeFront",
                    label: "R3 F",
                    color: "#FF0000"
                },
                {
                    prop: "r3RangeBack",
                    label: "R3 B",
                    color: "#EF0000"
                },
                {
                    prop: "r2RangeFront",
                    label: "R2 F",
                    color: "orange"
                },
                {
                    prop: "r2RangeBack",
                    label: "R2 B",
                    color: "magenta"
                },
                {
                    prop: "r1RangeFront",
                    label: "R1 F",
                    color: "#00FF00"
                },
                {
                    prop: "r1RangeBack",
                    label: "R1 B",
                    color: "#00EF00"
                },
            ]
        }*/
        Separator {} 
        HifiControls.Label {
            text: "Ranges & Numbers:";     
        }
        Repeater {
            model: [ 
                "green:R1:numR1:r1RangeBack:r1RangeFront",
                "orange:R2:numR2:r2RangeBack:r2RangeFront",
                "red:R3:numR3:r3RangeBack:r3RangeFront"
            ]
            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right 
                HifiControls.Label {
                    text: modelData.split(":")[1] + " : " + Workload.getConfig("regionState")[modelData.split(":")[2]] ;
                    color: modelData.split(":")[0]     
                }
                HifiControls.Label {
                    text: Workload.getConfig("controlViews")[modelData.split(":")[3]].toFixed(0) ;
                    color: modelData.split(":")[0]     
                }
                HifiControls.Label {
                    text: Workload.getConfig("controlViews")[modelData.split(":")[4]].toFixed(0) ;
                    color: modelData.split(":")[0]     
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
        Row {
            spacing: 5
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
            /*HifiControls.Button {
                text: "bump floor"
                onClicked: {
                    print("pressed")
                    _workload.broadcastBumpUpFloor()
                }
            }*/
        }
        HifiControls.Label {
            id: gridSizeLabel
            anchors.left: parent.left
            anchors.right: parent.right 
            text: "Grid side size [m]"                       
        }
        HifiControls.Slider {
            id: gridSize
            stepSize: 1.0
            anchors.left: parent.left
            anchors.right: parent.right 
            anchors.rightMargin: 0
            anchors.topMargin: 0
            minimumValue: 1
            maximumValue: 200
            value: 100

            onValueChanged: { _workload.broadcastChangeSize(value) }
        }

        HifiControls.Label {
            id: objectCount
            anchors.left: parent.left
            anchors.right: parent.right 
            text: "Num objects"                       
        }
        HifiControls.Slider {
            id: resolution
            stepSize: 1.0
            anchors.left: parent.left
            anchors.right: parent.right 
            anchors.rightMargin: 0
            anchors.topMargin: 0
            minimumValue: 5
            maximumValue: 75
            value: 5

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
