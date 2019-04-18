//
//  ShadingModel.qml
//
//  Created by Sam Gateau on 4/17/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import "../../lib/prop" as Prop


Column {

    id: shadingModel;   
    
    property var mainViewTask: Render.getConfig("RenderMainView")
    
    spacing: 5
    anchors.left: parent.left
    anchors.right: parent.right       
    anchors.margins: hifi.dimensions.contentMargin.x  
    HifiControls.Label {
                text: "Shading"       
    }
    Row {
        anchors.left: parent.left
        anchors.right: parent.right 
            
        spacing: 5
        Column { 
            spacing: 5
            Repeater {
                model: [
                        "Unlit:LightingModel:enableUnlit", 
                        "Emissive:LightingModel:enableEmissive", 
                        "Lightmap:LightingModel:enableLightmap",
                        "Background:LightingModel:enableBackground",      
                        "Haze:LightingModel:enableHaze",                        
                        "AO:LightingModel:enableAmbientOcclusion",
                        "Textures:LightingModel:enableMaterialTexturing"                     
                ]
                Prop.PropCheckBox {    
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
                Prop.PropCheckBox {   
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
                        "Shadow:LightingModel:enableShadow"
                ]
                Prop.PropCheckBox {
                    text: modelData.split(":")[0]
                    checked: render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]]
                    onCheckedChanged: { render.mainViewTask.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
                }
            }
        }
    }
}
