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
    Column {
        id: deferredLighting
        spacing: 10
        Repeater {
            model: [
                 "Unlit:LightingModel:enableUnlit", 
                 "Shaded:LightingModel:enableShaded", 
                 "Emissive:LightingModel:enableEmissive", 
                 "Lightmap:LightingModel:enableLightmap",
                 "Scattering:LightingModel:enableScattering",
                 "Diffuse:LightingModel:enableDiffuse",
                 "Specular:LightingModel:enableSpecular",
                 "Ambient:LightingModel:enableAmbientLight",
                 "Directional:LightingModel:enableDirectionalLight",
                 "Point:LightingModel:enablePointLight",
                 "Spot:LightingModel:enableSpotLight" 
            ]
            CheckBox {
                text: modelData.split(":")[0]
                checked: Render.getConfig(modelData.split(":")[1])
                onCheckedChanged: { Render.getConfig(modelData.split(":")[1])[modelData.split(":")[2]] = checked }
            }
        }
    }
}
