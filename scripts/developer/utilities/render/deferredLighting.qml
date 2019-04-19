//
//  deferredLighting.qml
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import  "configSlider"
import "luci"

Rectangle {
    HifiConstants { id: hifi;}
    id: render;   
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;
    property var mainViewTask: Render.getConfig("RenderMainView")
   
    Column {
        spacing: 5
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  
        
        
        HifiControls.Label {
                    text: "Shading"       
        }
        ShadingModel {}

        Separator {}  
        ToneMapping {}

        Separator {}          
        Column {
            anchors.left: parent.left
            anchors.right: parent.right 
            spacing: 5 
            Repeater {
                model: [ "MSAA:PrepareFramebuffer:numSamples:4:1"
                              ]
                ConfigSlider {
                        label: qsTr(modelData.split(":")[0])
                        integral: true
                        config: render.mainViewTask.getConfig(modelData.split(":")[1])
                        property: modelData.split(":")[2]
                        max: modelData.split(":")[3]
                        min: modelData.split(":")[4]

                        anchors.left: parent.left
                        anchors.right: parent.right 
                }
            }
        }
        Separator {}          
        Framebuffer {}
        
        Separator {}          
        BoundingBoxes {

        }
        Separator {}
        Row {
            HifiControls.Button {
                text: "Engine"
            // activeFocusOnPress: false
                onClicked: {
                sendToScript({method: "openEngineView"}); 
                }
            }
            HifiControls.Button {
                text: "LOD"
            // activeFocusOnPress: false
                onClicked: {
                sendToScript({method: "openEngineLODView"}); 
                }
            }
            HifiControls.Button {
                text: "Cull"
            // activeFocusOnPress: false
                onClicked: {
                sendToScript({method: "openCullInspectorView"}); 
                }
            }
        }
        Row {
            HifiControls.Button {
                text: "Material"
                onClicked: {
                sendToScript({method: "openMaterialInspectorView"}); 
                }
            }
        }
    }
}
