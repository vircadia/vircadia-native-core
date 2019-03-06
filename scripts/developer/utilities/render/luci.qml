//
//  luci.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import controlsUit 1.0 as HifiControls

import "../lib/prop" as Prop
import "../lib/jet/qml" as Jet

Rectangle {
    Prop.Global { id: prop;}
    id: render;   
    anchors.fill: parent
    color: prop.color;

    property var mainViewTask: Render.getConfig("RenderMainView")
   
    Column {
        anchors.left: parent.left
        anchors.right: parent.right       
      /*  Repeater {
            model: [ "Tone mapping exposure:ToneMapping:exposure:5.0:-5.0",
                     "Tone:ToneMapping:exposure:5.0:-5.0"
                        ]
            Prop.PropScalar {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    object: render.mainViewTask.getConfig(modelData.split(":")[1])
                    property: modelData.split(":")[2]
                    max: modelData.split(":")[3]
                    min: modelData.split(":")[4]

                    anchors.left: parent.left
                    anchors.right: parent.right 
            }
        }
        Prop.PropEnum {
            label: "Tone Curve"
            object: render.mainViewTask.getConfig("ToneMapping")
            property: "curve"
            enums: [
                        "RGB",
                        "SRGB",
                        "Reinhard",
                        "Filmic",
                    ]
            anchors.left: parent.left
            anchors.right: parent.right 
        }
        Prop.PropBool {
            label: "Background"
            object: render.mainViewTask.getConfig("LightingModel")
            property: "enableBackground"
            anchors.left: parent.left
            anchors.right: parent.right 
        }*/
     /*   Prop.PropGroup {
            label: "My group"
            propItems: [
                {"type": "PropBool", "object": render.mainViewTask.getConfig("LightingModel"), "property": "enableBackground"},
                {"type": "PropScalar", "object": render.mainViewTask.getConfig("ToneMapping"), "property": "exposure"},
                {"type": "PropBool", "object": render.mainViewTask.getConfig("LightingModel"), "property": "enableEmissive"},
                {"type": "PropEnum", "object": render.mainViewTask.getConfig("ToneMapping"), "property": "curve", enums: [
                        "RGB",
                        "SRGB",
                        "Reinhard",
                        "Filmic",
                    ]},
            ]
            anchors.left: parent.left
            anchors.right: parent.right 
        }*/

        Jet.TaskPropView {
            jobPath: "RenderMainView.LightingModel"
            label: "Le tone mapping Job"

            anchors.left: parent.left
            anchors.right: parent.right 
        }
    }
}