//
//  deferredLighting.qml
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"

Column {
    spacing: 8
    
    Row {
        spacing: 8
        Column {
            spacing: 10
            Repeater {
                model: [
                     "Unlit:LightingModel:enableUnlit", 
                     "Emissive:LightingModel:enableEmissive", 
                     "Lightmap:LightingModel:enableLightmap",
                     "Background:LightingModel:enableBackground",                      
                     "ssao:AmbientOcclusion:enabled",                      
                     "Textures:LightingModel:enableMaterialTexturing",                      
                ]
                CheckBox {
                    text: modelData.split(":")[0]
                    checked: Render.getConfig(modelData.split(":")[1])[modelData.split(":")[2]]
                    onCheckedChanged: { Render.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
                }
            }
        }


        Column {
            spacing: 10
            Repeater {
                model: [
                     "Obscurance:LightingModel:enableObscurance",
                     "Scattering:LightingModel:enableScattering",
                     "Diffuse:LightingModel:enableDiffuse",
                     "Specular:LightingModel:enableSpecular",
                     "Albedo:LightingModel:enableAlbedo",
                ]
                CheckBox {
                    text: modelData.split(":")[0]
                    checked: Render.getConfig(modelData.split(":")[1])[modelData.split(":")[2]]
                    onCheckedChanged: { Render.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
                }
            }
        }

        Column {
            spacing: 10
            Repeater {
                model: [
                     "Ambient:LightingModel:enableAmbientLight",
                     "Directional:LightingModel:enableDirectionalLight",
                     "Point:LightingModel:enablePointLight",
                     "Spot:LightingModel:enableSpotLight",
                     "Light Contour:LightingModel:showLightContour"
                ]
                CheckBox {
                    text: modelData.split(":")[0]
                    checked: Render.getConfig(modelData.split(":")[1])[modelData.split(":")[2]]
                    onCheckedChanged: { Render.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
                }
            }
        }
    }
    Column {
        spacing: 10 
        Repeater {
            model: [ "Tone Mapping Exposure:ToneMapping:exposure:5.0:-5.0"
                          ]
            ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    config: Render.getConfig(modelData.split(":")[1])
                    property: modelData.split(":")[2]
                    max: modelData.split(":")[3]
                    min: modelData.split(":")[4]
            }
        }

        Row {
            Label {
                text: "Tone Mapping Curve"
                anchors.left: root.left           
            }

            ComboBox {
                anchors.right: root.right           
                currentIndex: 1
                model: ListModel {
                    id: cbItems
                    ListElement { text: "RGB"; color: "Yellow" }
                    ListElement { text: "SRGB"; color: "Green" }
                    ListElement { text: "Reinhard"; color: "Yellow" }
                    ListElement { text: "Filmic"; color: "White" }
                }
                width: 200
                onCurrentIndexChanged: { Render.getConfig("ToneMapping")["curve"] = currentIndex }
            }
        }
    }
    Row {
        id: framebuffer
        spacing: 10 

        Label {
            text: "Debug Framebuffer"
            anchors.left: root.left           
        }
        
        property var config: Render.getConfig("DebugDeferredBuffer")

        function setDebugMode(mode) {
            framebuffer.config.enabled = (mode != 0);
            framebuffer.config.mode = mode;
        }

        ComboBox {
            anchors.right: root.right           
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
                ListElement { text: "Shadow"; color: "White" }
                ListElement { text: "Linear Depth"; color: "White" }
                ListElement { text: "Half Linear Depth"; color: "White" }
                ListElement { text: "Half Normal"; color: "White" }
                ListElement { text: "Mid Curvature"; color: "White" }
                ListElement { text: "Mid Normal"; color: "White" }
                ListElement { text: "Low Curvature"; color: "White" }
                ListElement { text: "Low Normal"; color: "White" }
                ListElement { text: "Debug Scattering"; color: "White" }
                ListElement { text: "Ambient Occlusion"; color: "White" }
                ListElement { text: "Ambient Occlusion Blurred"; color: "White" }
                ListElement { text: "Custom"; color: "White" }
            }
            width: 200
            onCurrentIndexChanged: { framebuffer.setDebugMode(currentIndex) }
        }
    }

    Column {
        id: metas
        CheckBox {
            text: "Metas"
            checked: Render.getConfig("DrawMetaBounds")["enabled"]
            onCheckedChanged: { Render.getConfig("DrawMetaBounds")["enabled"] = checked }
        }
        CheckBox {
            text: "Opaques"
            checked: Render.getConfig("DrawOpaqueBounds")["enabled"]
            onCheckedChanged: { Render.getConfig("DrawOpaqueBounds")["enabled"] = checked }
        }
        CheckBox {
            text: "Transparents"
            checked: Render.getConfig("DrawTransparentBounds")["enabled"]
            onCheckedChanged: { Render.getConfig("DrawTransparentBounds")["enabled"] = checked }
        }
        CheckBox {
            text: "Overlay Opaques"
            checked: Render.getConfig("DrawOverlayOpaqueBounds")["enabled"]
            onCheckedChanged: { Render.getConfig("DrawOverlayOpaqueBounds")["enabled"] = checked }
        }
        CheckBox {
            text: "Overlay Transparents"
            checked: Render.getConfig("DrawOverlayTransparentBounds")["enabled"]
            onCheckedChanged: { Render.getConfig("DrawOverlayTransparentBounds")["enabled"] = checked }
        }
    }
}

