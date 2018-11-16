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
import "../lib/jet/qml" as Jet

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
        //padding: hifi.dimensions.contentMargin.x
        HifiControls.Label {
                    text: "Shading"       
        }
        Row {
            anchors.left: parent.left
            anchors.right: parent.right 
              
            spacing: 5
            Column { 
                spacing: 5
           // padding: 10
                Repeater {
                    model: [
                         "Unlit:LightingModel:enableUnlit", 
                         "Emissive:LightingModel:enableEmissive", 
                         "Lightmap:LightingModel:enableLightmap",
                         "Background:LightingModel:enableBackground",      
                         "Haze:LightingModel:enableHaze",                
                         "ssao:AmbientOcclusion:enabled",                      
                         "Textures:LightingModel:enableMaterialTexturing"                     
                    ]
                    HifiControls.CheckBox {
                        boxSize: 20
                        text: modelData.split(":")[0]
                        checked: render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]]
                        onCheckedChanged: { render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
                    }
                }
            }


            Column {
                spacing: 5
                Repeater {
                    model: [
                         "Obscurance:LightingModel:enableObscurance",
                         "Scattering:LightingModel:enableScattering",
                         "Diffuse:LightingModel:enableDiffuse",
                         "Specular:LightingModel:enableSpecular",
                         "Albedo:LightingModel:enableAlbedo",
                         "Wireframe:LightingModel:enableWireframe",
                         "Skinning:LightingModel:enableSkinning",
                         "Blendshape:LightingModel:enableBlendshape"
                    ]
                    HifiControls.CheckBox {
                        boxSize: 20
                        text: modelData.split(":")[0]
                        checked: render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]]
                        onCheckedChanged: { render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
                    }
                }
            }

            Column {
                spacing: 5
                Repeater {
                    model: [
                         "Ambient:LightingModel:enableAmbientLight",
                         "Directional:LightingModel:enableDirectionalLight",
                         "Point:LightingModel:enablePointLight",
                         "Spot:LightingModel:enableSpotLight",
                         "Light Contour:LightingModel:showLightContour",
                         "Zone Stack:DrawZoneStack:enabled",
                         "Shadow:RenderShadowTask:enabled"
                    ]
                    HifiControls.CheckBox {
                        boxSize: 20
                        text: modelData.split(":")[0]
                        checked: render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]]
                        onCheckedChanged: { render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
                    }
                }
            }
        }
        Separator {}          
        Column {
            anchors.left: parent.left
            anchors.right: parent.right 
            spacing: 5 
            Repeater {
                model: [ "Tone Mapping Exposure:ToneMapping:exposure:5.0:-5.0"
                              ]
                ConfigSlider {
                        label: qsTr(modelData.split(":")[0])
                        integral: false
                        config: render.mainViewTask.getConfig(modelData.split(":")[1])
                        property: modelData.split(":")[2]
                        max: modelData.split(":")[3]
                        min: modelData.split(":")[4]

                        anchors.left: parent.left
                        anchors.right: parent.right 
                }
            }

            Item {
                height: childrenRect.height
                anchors.left: parent.left
                anchors.right: parent.right 

                HifiControls.Label {
                    text: "Tone Mapping Curve"
                    anchors.left: parent.left           
                }

                HifiControls.ComboBox {
                    anchors.right: parent.right           
                    currentIndex: 1
                    model: ListModel {
                        id: cbItems
                        ListElement { text: "RGB"; color: "Yellow" }
                        ListElement { text: "SRGB"; color: "Green" }
                        ListElement { text: "Reinhard"; color: "Yellow" }
                        ListElement { text: "Filmic"; color: "White" }
                    }
                    width: 200
                    onCurrentIndexChanged: { render.mainViewTask.getConfig("ToneMapping")["curve"] = currentIndex }
                }
            }
        }
        Separator {}          
        
        Item {
            height: childrenRect.height
            anchors.left: parent.left
            anchors.right: parent.right 

            id: framebuffer

            HifiControls.Label {
                text: "Debug Framebuffer"
                anchors.left: parent.left           
            }
            
            property var config: render.mainViewTask.getConfig("DebugDeferredBuffer")

            function setDebugMode(mode) {
                framebuffer.config.enabled = (mode != 0);
                framebuffer.config.mode = mode;
            }

            HifiControls.ComboBox {
                anchors.right: parent.right           
                currentIndex: 0
                model: ListModel {
                    id: cbItemsFramebuffer
                    ListElement { text: "Off"; color: "Yellow" }
                    ListElement { text: "Depth"; color: "Green" }
                    ListElement { text: "Albedo"; color: "Yellow" }
                    ListElement { text: "Normal"; color: "White" }
                    ListElement { text: "Roughness"; color: "White" }
                    ListElement { text: "Metallic"; color: "White" }
                    ListElement { text: "Emissive"; color: "White" }
                    ListElement { text: "Unlit"; color: "White" }
                    ListElement { text: "Occlusion"; color: "White" }
                    ListElement { text: "Lightmap"; color: "White" }
                    ListElement { text: "Scattering"; color: "White" }
                    ListElement { text: "Lighting"; color: "White" }
                    ListElement { text: "Shadow Cascade 0"; color: "White" }
                    ListElement { text: "Shadow Cascade 1"; color: "White" }
                    ListElement { text: "Shadow Cascade 2"; color: "White" }
                    ListElement { text: "Shadow Cascade 3"; color: "White" }
                    ListElement { text: "Shadow Cascade Indices"; color: "White" }
                    ListElement { text: "Linear Depth"; color: "White" }
                    ListElement { text: "Half Linear Depth"; color: "White" }
                    ListElement { text: "Half Normal"; color: "White" }
                    ListElement { text: "Mid Curvature"; color: "White" }
                    ListElement { text: "Mid Normal"; color: "White" }
                    ListElement { text: "Low Curvature"; color: "White" }
                    ListElement { text: "Low Normal"; color: "White" }
                    ListElement { text: "Curvature Occlusion"; color: "White" }
                    ListElement { text: "Debug Scattering"; color: "White" }
                    ListElement { text: "Ambient Occlusion"; color: "White" }
                    ListElement { text: "Ambient Occlusion Blurred"; color: "White" }
                    ListElement { text: "Ambient Occlusion Normal"; color: "White" }
                    ListElement { text: "Velocity"; color: "White" }
                    ListElement { text: "Custom"; color: "White" }
                }
                width: 200
                onCurrentIndexChanged: { framebuffer.setDebugMode(currentIndex) }
            }
        }

        Separator {}          
        Row {
            spacing: 5 
            Column {
                spacing: 5 

                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Opaques"
                    checked: render.mainViewTask.getConfig("DrawOpaqueBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawOpaqueBounds")["enabled"] = checked }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Transparents"
                    checked: render.mainViewTask.getConfig("DrawTransparentBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawTransparentBounds")["enabled"] = checked }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Opaques in Front"
                    checked: render.mainViewTask.getConfig("DrawOverlayInFrontOpaqueBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawOverlayInFrontOpaqueBounds")["enabled"] = checked }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Transparents in Front"
                    checked: render.mainViewTask.getConfig("DrawOverlayInFrontTransparentBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawOverlayInFrontTransparentBounds")["enabled"] = checked }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Opaques in HUD"
                    checked: render.mainViewTask.getConfig("DrawOverlayHUDOpaqueBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawOverlayHUDOpaqueBounds")["enabled"] = checked }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Transparents in HUD"
                    checked: render.mainViewTask.getConfig("DrawOverlayHUDTransparentBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawOverlayHUDTransparentBounds")["enabled"] = checked }
                }

            }
            Column {
                spacing: 5 
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Metas"
                    checked: render.mainViewTask.getConfig("DrawMetaBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawMetaBounds")["enabled"] = checked }
                }   
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Lights"
                    checked: render.mainViewTask.getConfig("DrawLightBounds")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("DrawLightBounds")["enabled"] = checked; }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Zones"
                    checked: render.mainViewTask.getConfig("DrawZones")["enabled"]
                    onCheckedChanged: { render.mainViewTask.getConfig("ZoneRenderer")["enabled"] = checked; render.mainViewTask.getConfig("DrawZones")["enabled"] = checked; }
                }
            }
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
    }
}
