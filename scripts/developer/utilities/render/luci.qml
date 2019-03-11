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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import controlsUit 1.0 as HifiControls

import "../lib/prop" as Prop
import "../lib/jet/qml" as Jet

Rectangle {
    anchors.fill: parent 
    id: render;   
    property var mainViewTask: Render.getConfig("RenderMainView")
    
    Prop.Global { id: global;}
    color: global.color

    ScrollView {
        id: control
        anchors.fill: parent 
        clip: true
         
        Column {
            width: render.width

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
        
            Jet.TaskPropView {
                jobPath: "RenderMainView.ToneMapping"
                label: "Le ToneMapping Job"

                anchors.left: parent.left
                anchors.right: parent.right 
            }
            Jet.TaskPropView {
                jobPath: "RenderMainView.Antialiasing"
                label: "Le Antialiasing Job"

                anchors.left: parent.left
                anchors.right: parent.right 
            }
        }
    }
}