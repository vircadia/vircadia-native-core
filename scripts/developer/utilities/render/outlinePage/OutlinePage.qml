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
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import "../configSlider"
import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls

Rectangle {
    id: root
    property var outlineIndex: 0
    property var drawConfig: Render.getConfig("RenderMainView.OutlineEffect"+outlineIndex)

    HifiConstants { id: hifi;}
    anchors.margins: hifi.dimensions.contentMargin.x

    Column {
        spacing: 5
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  

        HifiControls.CheckBox {
            id: glow
            text: "Glow"
            checked: root.drawConfig["glow"]
            onCheckedChanged: {
                root.drawConfig["glow"] = checked;
            }
        }
        Repeater {
            model: ["Width:width:5.0:0.0",
                    "Intensity:intensity:1.0:0.0"
                            ]
            ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    config: root.drawConfig
                    property: modelData.split(":")[1]
                    max: modelData.split(":")[2]
                    min: modelData.split(":")[3]

                    anchors.left: parent.left
                    anchors.right: parent.right 
            }
        }

        GroupBox {
            title: "Color"
            anchors.left: parent.left
            anchors.right: parent.right       
            Column {
                spacing: 10
                anchors.left: parent.left
                anchors.right: parent.right       
                anchors.margins: hifi.dimensions.contentMargin.x  

                Repeater {
                    model: ["Red:colorR:1.0:0.0",
                            "Green:colorG:1.0:0.0",
                            "Blue:colorB:1.0:0.0"
                                    ]
                    ConfigSlider {
                            label: qsTr(modelData.split(":")[0])
                            integral: false
                            config: root.drawConfig
                            property: modelData.split(":")[1]
                            max: modelData.split(":")[2]
                            min: modelData.split(":")[3]

                            anchors.left: parent.left
                            anchors.right: parent.right 
                    }
                }
            }
        }

        GroupBox {
            title: "Fill Opacity"
            anchors.left: parent.left
            anchors.right: parent.right       
            Column {
                spacing: 10
                anchors.left: parent.left
                anchors.right: parent.right       
                anchors.margins: hifi.dimensions.contentMargin.x  

                Repeater {
                    model: ["Unoccluded:unoccludedFillOpacity:1.0:0.0",
                            "Occluded:occludedFillOpacity:1.0:0.0"
                                    ]
                    ConfigSlider {
                            label: qsTr(modelData.split(":")[0])
                            integral: false
                            config: root.drawConfig
                            property: modelData.split(":")[1]
                            max: modelData.split(":")[2]
                            min: modelData.split(":")[3]

                            anchors.left: parent.left
                            anchors.right: parent.right 
                    }
                }
            }
        }
    }
}
