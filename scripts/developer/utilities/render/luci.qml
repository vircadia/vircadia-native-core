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
import "luci"

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
            Prop.PropFolderPanel {
                id: "shadingModel"
                label: "Shading Model"
                panelFrameData: Component {
                    ShadingModel {
                    }
                }
            }
        /*    Prop.PropEnum {
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
            }        */
            Jet.TaskPropView {
                id: "theView"
                jobPath: "RenderMainView"
                label: "Le Render Main View"

                anchors.left: parent.left
                anchors.right: parent.right 
            }
            Jet.TaskPropView {
                id: "the"
                jobPath: "RenderMainView.RenderDeferredTask"
                label: "Le Render Deferred Job"

                anchors.left: parent.left
                anchors.right: parent.right 
            }
            Jet.TaskPropView {
                id: "le"
                jobPath: ""
                label: "Le Render Engine"

                anchors.left: parent.left
                anchors.right: parent.right 
            }
        }
    }

    Component.onCompleted: {
     }
}