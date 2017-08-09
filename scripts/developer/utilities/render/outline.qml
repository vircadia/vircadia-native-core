//
//  outline.qml
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
import "configSlider"

Item {
    id: root
    property var pickConfig: Render.getConfig("RenderMainView.PickOutlined")
    property var debugConfig: Render.getConfig("RenderMainView.DebugOutline")
    property var drawConfig: Render.getConfig("RenderMainView.DrawOutline")

    Column {
        spacing: 8

        CheckBox {
            text: "Edit Outline"
            checked: root.pickConfig["pick"]
            onCheckedChanged: {
                root.pickConfig["pick"] = checked;
            }
        }
        CheckBox {
            text: "View Outlined Depth"
            checked: root.debugConfig["viewOutlinedDepth"]
            onCheckedChanged: {
                root.debugConfig["viewOutlinedDepth"] = checked;
            }
        }
        ConfigSlider {
            label: "Width"
            integral: false
            config: root.drawConfig
            property: "width"
            max: 15.0
            min: 0.0
            width: 230
        }  
        ConfigSlider {
            label: "Color R"
            integral: false
            config: root.drawConfig
            property: "colorR"
            max: 1.0
            min: 0.0
            width: 230
        }  
        ConfigSlider {
            label: "Color G"
            integral: false
            config: root.drawConfig
            property: "colorG"
            max: 1.0
            min: 0.0
            width: 230
        }  
        ConfigSlider {
            label: "Color B"
            integral: false
            config: root.drawConfig
            property: "colorB"
            max: 1.0
            min: 0.0
            width: 230
        }  
    }
}
