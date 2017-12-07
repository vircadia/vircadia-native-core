//
//  haze.qml
//  developer/utilities/render
//
//  Nissim Hadar, created on 9/8/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../utilities/render/configSlider"

Item {
    id: root
    property var hazeModel: Render.getConfig("RenderMainView.HazeModel")

    Column {
        spacing: 8

        Row {
          CheckBox {
              text: "Haze Active"
              checked: root.hazeModel["isHazeActive"]
              onCheckedChanged: {
                  root.hazeModel["isHazeActive"] = checked;
              }
          }

          CheckBox {
              text: "Modulated Color"
              checked: root.hazeModel["isModulateColorActive"]
              onCheckedChanged: {
                  root.hazeModel["isModulateColorActive"] = checked;
              }
          }
        }
        CheckBox {
            text: "Altitude Based"
            checked: root.hazeModel["isAltitudeBased"]
            onCheckedChanged: {
                root.hazeModel["isAltitudeBased"] = checked;
            }
        }
        
        ConfigSlider {
            label: "Range"
            integral: false
            config: root.hazeModel
            property: "hazeRange_m"
            max: 10000
            min: 5
            width: 280
        }
        
        ConfigSlider {
            label: "Altitude"
            integral: false
            config: root.hazeModel
            property: "hazeAltitude_m"
            max: 2000
            min: 5
            width: 280
        }
        
        ConfigSlider {
            label: "Haze Color R"
            integral: false
            config: root.hazeModel
            property: "hazeColor.r"
            max: 1.0
            min: 0.0
            width: 280
        }
        
        ConfigSlider {
            label: "Haze Color G"
            integral: false
            config: root.hazeModel
            property: "hazeColor.g"
            max: 1.0
            min: 0.0
            width: 280
        }
        
        ConfigSlider {
            label: "Haze Color B"
            integral: false
            config: root.hazeModel
            property: "hazeColor.b"
            max: 1.0
            min: 0.0
            width: 280
        }
        
        ConfigSlider {
            label: "Sun R"
            integral: false
            config: root.hazeModel
            property: "hazeGlareColor.r"
            max: 1.0
            min: 0.0
            width: 280
        }
        
        ConfigSlider {
            label: "Sun G"
            integral: false
            config: root.hazeModel
            property: "hazeGlareColor.g"
            max: 1.0
            min: 0.0
            width: 280
        }
        
        ConfigSlider {
            label: "Sun B"
            integral: false
            config: root.hazeModel
            property: "hazeGlareColor.b"
            max: 1.0
            min: 0.0
            width: 280
        }
        
        ConfigSlider {
            label: "Sun glare angle"
            integral: false
            config: root.hazeModel
            property: "hazeGlareAngle_degs"
            max: 70.0
            min: 0.0
            width: 280
        }
        
        ConfigSlider {
            label: "Base"
            integral: false
            config: root.hazeModel
            property: "hazeBaseReference"
            max: 500.0
            min: -500.0
            width: 280
        }
        
        ConfigSlider {
            label: "BG Blend"
            integral: false
            config: root.hazeModel
            property: "hazeBackgroundBlend"
            max: 1.0
            min: 0.0
            width: 280
        }

        CheckBox {
            text: "Keylight Attenuation"
            checked: root.hazeModel["isKeyLightAttenuationActive"]
            onCheckedChanged: {
                root.hazeModel["isKeyLightAttenuationActive"] = checked;
            }
        }
        
        ConfigSlider {
            label: "Range"
            integral: false
            config: root.hazeModel
            property: "hazeKeyLightRange_m"
            max: 10000
            min: 5
            width: 280
        }
        
        ConfigSlider {
            label: "Altitude"
            integral: false
            config: root.hazeModel
            property: "hazeKeyLightAltitude_m"
            max: 2000
            min: 5
            width: 280
        }
    }
}
