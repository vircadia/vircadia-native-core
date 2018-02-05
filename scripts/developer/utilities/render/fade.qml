//
//  fade.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 30/04/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls
import  "configSlider"
import "../lib/plotperf"

Rectangle {
    HifiConstants { id: hifi;}
    id: root
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;

    property var config: Render.getConfig("RenderMainView.Fade");
    property var configEdit: Render.getConfig("RenderMainView.FadeEdit");

    Column {
        spacing: 5
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  
        HifiControls.Label {
            text: "Fade"       
        }

        Row {
            anchors.left: parent.left
            anchors.right: parent.right 
            spacing: 20

            HifiControls.CheckBox {
                boxSize: 20
                text: "Edit"
                checked: root.configEdit["editFade"]
                onCheckedChanged: {
                    root.configEdit["editFade"] = checked;
                    Render.getConfig("RenderMainView.DrawFadedOpaqueBounds").enabled = checked;
                }
            }
            HifiControls.ComboBox {
                id: categoryBox
                width: 260
                model: ["Elements enter/leave domain", "Bubble isect. - Owner POV", "Bubble isect. - Trespasser POV", "Another user leaves/arrives", "Changing an avatar"]
                Timer {
                    id: postpone
                    interval: 100; running: false; repeat: false
                    onTriggered: { paramWidgetLoader.sourceComponent = paramWidgets }
                }
                onCurrentIndexChanged: {
                    root.config["editedCategory"] = currentIndex;
                    // This is a hack to be sure the widgets below properly reflect the change of category: delete the Component
                    // by setting the loader source to Null and then recreate it 100ms later
                    paramWidgetLoader.sourceComponent = undefined;
                    postpone.interval = 100
                    postpone.start()
                }
            }
            HifiControls.CheckBox {
                boxSize: 20
                text: "Manual"
                checked: root.config["manualFade"]
                onCheckedChanged: {
                    root.config["manualFade"] = checked;
                }
            }
        }

        ConfigSlider {
            height: 35
            label: "Threshold"
            integral: false
            config: root.config
            property: "manualThreshold"
            max: 1.0
            min: 0.0
            anchors.left: parent.left
            anchors.right: parent.right
        }
        
        Action {
            id: saveAction
            text: "Save"
            onTriggered: {
                root.config.save()
            }
        }
        Action {
            id: loadAction
            text: "Load"
            onTriggered: {
                root.config.load()
                // This is a hack to be sure the widgets below properly reflect the change of category: delete the Component
                // by setting the loader source to Null and then recreate it 500ms later
                paramWidgetLoader.sourceComponent = undefined;
                postpone.interval = 500
                postpone.start()
            }
        }

              
        Component {
            id: paramWidgets

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 10

                HifiControls.CheckBox {
                    text: "Invert"
                    checked: root.config["isInverted"]
                    onCheckedChanged: { root.config["isInverted"] = checked }
                }
                GroupBox {
                    title: "Base Gradient"
                    anchors.left: parent.left
                    anchors.right: parent.right

                    Column {
                        spacing: 8
                        anchors.left: parent.left
                        anchors.right: parent.right

                        Repeater {
                            model: [
                            "Size X:baseSizeX", 
                            "Size Y:baseSizeY", 
                            "Size Z:baseSizeZ",
                            "Level:baseLevel" ]                  

                            ConfigSlider {
                                height: 35
                                label:  modelData.split(":")[0]
                                integral: false
                                config: root.config
                                property:  modelData.split(":")[1]
                                max: 1.0
                                min: 0.0
                            }
                        }
                    }
                }
                GroupBox {
                    title: "Noise Gradient"
                    
                    Column {
                        spacing: 8
                        anchors.left: parent.left
                        anchors.right: parent.right

                        ConfigSlider {
                            height: 35
                            label: "Size X"
                            integral: false
                            config: root.config
                            property: "noiseSizeX"
                            max: 1.0
                            min: 0.0
                        }
                        ConfigSlider {
                            height: 35
                            label: "Size Y"
                            integral: false
                            config: root.config
                            property: "noiseSizeY"
                            max: 1.0
                            min: 0.0
                        }
                        ConfigSlider {
                            label: "Size Z"
                            integral: false
                            config: root.config
                            property: "noiseSizeZ"
                            max: 1.0
                            min: 0.0
                        }
                        ConfigSlider {
                            label: "Level"
                            integral: false
                            config: root.config
                            property: "noiseLevel"
                            max: 1.0
                            min: 0.0
                        }
                    }
                }
                GroupBox {
                    title: "Edge"

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right

                        ConfigSlider {
                            label: "Width"
                            integral: false
                            config: root.config
                            property: "edgeWidth"
                            max: 1.0
                            min: 0.0
                        }
                        GroupBox {
                            title: "Inner color"
                            Column {
                                anchors.left: parent.left
                                anchors.right: parent.right

                                ConfigSlider {
                                    label: "Color R"
                                    integral: false
                                    config: root.config
                                    property: "edgeInnerColorR"
                                    max: 1.0
                                    min: 0.0
                                }
                                ConfigSlider {
                                    label: "Color G"
                                    integral: false
                                    config: root.config
                                    property: "edgeInnerColorG"
                                    max: 1.0
                                    min: 0.0
                                }
                                ConfigSlider {
                                    label: "Color B"
                                    integral: false
                                    config: root.config
                                    property: "edgeInnerColorB"
                                    max: 1.0
                                    min: 0.0
                                }
                                ConfigSlider {
                                    label: "Color intensity"
                                    integral: false
                                    config: root.config
                                    property: "edgeInnerIntensity"
                                    max: 5.0
                                    min: 0.0
                                }
                            }
                        }
                        GroupBox {
                            title: "Outer color"
                            Column {
                                anchors.left: parent.left
                                anchors.right: parent.right 

                                ConfigSlider {
                                    label: "Color R"
                                    integral: false
                                    config: root.config
                                    property: "edgeOuterColorR"
                                    max: 1.0
                                    min: 0.0
                                }
                                ConfigSlider {
                                    label: "Color G"
                                    integral: false
                                    config: root.config
                                    property: "edgeOuterColorG"
                                    max: 1.0
                                    min: 0.0
                                }
                                ConfigSlider {
                                    label: "Color B"
                                    integral: false
                                    config: root.config
                                    property: "edgeOuterColorB"
                                    max: 1.0
                                    min: 0.0
                                }
                                ConfigSlider {
                                    label: "Color intensity"
                                    integral: false
                                    config: root.config
                                    property: "edgeOuterIntensity"
                                    max: 5.0
                                    min: 0.0
                                }
                            }
                        }
                    }
                }

                GroupBox {
                    title: "Timing"

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right 

                        ConfigSlider {
                            label: "Duration"
                            integral: false
                            config: root.config
                            property: "duration"
                            max: 10.0
                            min: 0.1
                        }
                        HifiControls.ComboBox {
                            width: 200
                            model: ["Linear", "Ease In", "Ease Out", "Ease In / Out"]
                            currentIndex: root.config["timing"]
                            onCurrentIndexChanged: {
                                root.config["timing"] = currentIndex;
                            }
                        }
                        GroupBox {
                            title: "Noise Animation"
                            Column {
                                anchors.left: parent.left
                                anchors.right: parent.right 

                                ConfigSlider {
                                    label: "Speed X"
                                    integral: false
                                    config: root.config
                                    property: "noiseSpeedX"
                                    max: 1.0
                                    min: -1.0
                                }
                                ConfigSlider {
                                    label: "Speed Y"
                                    integral: false
                                    config: root.config
                                    property: "noiseSpeedY"
                                    max: 1.0
                                    min: -1.0
                                }
                                ConfigSlider {
                                    label: "Speed Z"
                                    integral: false
                                    config: root.config
                                    property: "noiseSpeedZ"
                                    max: 1.0
                                    min: -1.0
                                }
                            }
                        }

                        PlotPerf {
                            title: "Threshold"
                            height: parent.evalEvenHeight()
                            object:  config
                            valueUnit: "%"
                            valueScale: 0.01
                            valueNumDigits: "1"
                            plots: [
                                {
                                    prop: "threshold",
                                    label: "Threshold",
                                    color: "#FFBB77"
                                }
                            ]
                        }

                    }
                }

            }
        }




        Loader {
            id: paramWidgetLoader
            sourceComponent: paramWidgets
        }

        Row {
            anchors.left: parent.left
            anchors.right: parent.right 

            Button {
                action: saveAction
            }
            Button {
                action: loadAction
            }
        }


        
    }
}