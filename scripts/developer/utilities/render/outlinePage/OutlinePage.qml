//
//  outlinePage.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 08/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../configSlider"

Item {
    id: root
    property var outlineIndex: 0
    property var drawConfig: Render.getConfig("RenderMainView.OutlineEffect"+outlineIndex)

    Column {
        spacing: 8

        CheckBox {
            id: glow
            text: "Glow"
            checked: root.drawConfig["glow"]
            onCheckedChanged: {
                root.drawConfig["glow"] = checked;
            }
        }
        ConfigSlider {
            label: "Width"
            integral: false
            config: root.drawConfig
            property: "width"
            max: 15.0
            min: 0.0
            width: 280
        }  
        ConfigSlider {
            label: "Intensity"
            integral: false
            config: root.drawConfig
            property: "intensity"
            max: 1.0
            min: 0.0
            width: 280
        }  

        GroupBox {
            title: "Color"
            width: 280
            Column {
                spacing: 8

                ConfigSlider {
                    label: "Red"
                    integral: false
                    config: root.drawConfig
                    property: "colorR"
                    max: 1.0
                    min: 0.0
                    width: 270
                }  
                ConfigSlider {
                    label: "Green"
                    integral: false
                    config: root.drawConfig
                    property: "colorG"
                    max: 1.0
                    min: 0.0
                    width: 270
                }  
                ConfigSlider {
                    label: "Blue"
                    integral: false
                    config: root.drawConfig
                    property: "colorB"
                    max: 1.0
                    min: 0.0
                    width: 270
                }
            }
        }

        GroupBox {
            title: "Fill Opacity"
            width: 280
            Column {
                spacing: 8

                ConfigSlider {
                    label: "Unoccluded"
                    integral: false
                    config: root.drawConfig
                    property: "unoccludedFillOpacity"
                    max: 1.0
                    min: 0.0
                    width: 270
                }  
                ConfigSlider {
                    label: "Occluded"
                    integral: false
                    config: root.drawConfig
                    property: "occludedFillOpacity"
                    max: 1.0
                    min: 0.0
                    width: 270
                }
            }
        }
    }
}
