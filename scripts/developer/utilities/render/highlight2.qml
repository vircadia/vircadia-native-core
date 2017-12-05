//
//  highlight.qml
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

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls
import  "configSlider"
import  "../lib/plotperf"

Rectangle {
    id: root
    HifiConstants { id: hifi;}
    color: hifi.colors.baseGray;
    anchors.margins: hifi.dimensions.contentMargin.x

    //property var debugConfig: Render.getConfig("RenderMainView.HighlightDebug")
    //property var highlightConfig: Render.getConfig("UpdateScene.HighlightStageSetup")
    
    signal sendToScript(var message);

    Column {
        id: col
        spacing: 10
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  

        Row {
            spacing: 10
            anchors.left: parent.left
            anchors.right: parent.right 

            HifiControls.CheckBox {
                id: debug
                text: "View Mask"
                checked: root.debugConfig["viewMask"]
                onCheckedChanged: {
                    root.debugConfig["viewMask"] = checked;
                }
            }
            HifiControls.CheckBox {
                text: "Hover select"
                checked: false
                onCheckedChanged: {
                    sendToScript("pick "+checked.toString())
                }
            }
            HifiControls.CheckBox {
                text: "Add to selection"
                checked: false
                onCheckedChanged: {
                    sendToScript("add "+checked.toString())
                }
            }        
        }
        Separator {}

        XColor {
            color: { "red": 0, "green": 255, "blue": 0}
        }

        Separator {}

/*
        HifiControls.ComboBox {
            id: box
            width: 350
            z: 999
            editable: true
            colorScheme: hifi.colorSchemes.dark
            model: [
                "contextOverlayHighlightList", 
                "highlightList1", 
                "highlightList2", 
                "highlightList3", 
                "highlightList4"]
            label: ""

            Timer {
                id: postpone
                interval: 100; running: false; repeat: false
                onTriggered: { paramWidgetLoader.sourceComponent = paramWidgets }
            }
            onCurrentIndexChanged: {
                root.highlightConfig["selectionName"] = model[currentIndex];
                sendToScript("highlight "+currentIndex)
                // This is a hack to be sure the widgets below properly reflect the change of category: delete the Component
                // by setting the loader source to Null and then recreate it 100ms later
                paramWidgetLoader.sourceComponent = undefined;
                postpone.interval = 100
                postpone.start()
            }
        }

        Loader {
            id: paramWidgetLoader
            sourceComponent: paramWidgets
            width: 350
        }

        Component {
            id: paramWidgets

            Column {
                spacing: 10
                anchors.margins: hifi.dimensions.contentMargin.x  

                HifiControls.Label {
                    text: "Outline"       
                }
                Column {
                    spacing: 10
                    anchors.left: parent.left
                    anchors.right: parent.right
                    HifiControls.CheckBox {
                        text: "Smooth"
                        checked: root.highlightConfig["isOutlineSmooth"]
                        onCheckedChanged: {
                            root.highlightConfig["isOutlineSmooth"] = checked;
                        }
                    }
                    Repeater {
                        model: ["Width:outlineWidth:5.0:0.0",
                                "Intensity:outlineIntensity:1.0:0.0"
                                        ]
                        ConfigSlider {
                                label: qsTr(modelData.split(":")[0])
                                integral: false
                                config: root.highlightConfig
                                property: modelData.split(":")[1]
                                max: modelData.split(":")[2]
                                min: modelData.split(":")[3]
                        }
                    }
                }

                Separator {}
                HifiControls.Label {
                    text: "Color"       
                }
                Repeater {
                    model: ["Red:colorR:1.0:0.0",
                            "Green:colorG:1.0:0.0",
                            "Blue:colorB:1.0:0.0"
                                    ]
                    ConfigSlider {
                            label: qsTr(modelData.split(":")[0])
                            integral: false
                            config: root.highlightConfig
                            property: modelData.split(":")[1]
                            max: modelData.split(":")[2]
                            min: modelData.split(":")[3]
                    }
                }

                Separator {}
                HifiControls.Label {
                    text: "Fill Opacity"       
                }
                Repeater {
                    model: ["Unoccluded:unoccludedFillOpacity:1.0:0.0",
                            "Occluded:occludedFillOpacity:1.0:0.0"
                                    ]
                    ConfigSlider {
                            label: qsTr(modelData.split(":")[0])
                            integral: false
                            config: root.highlightConfig
                            property: modelData.split(":")[1]
                            max: modelData.split(":")[2]
                            min: modelData.split(":")[3]
                    }
                }
            }
        }
        */
    }
}
