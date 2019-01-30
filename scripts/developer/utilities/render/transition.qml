//
//  transition.qml
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
import QtQuick.Dialogs 1.0

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import  "configSlider"
import "../lib/plotperf"

Rectangle {
    HifiConstants { id: hifi;}
    id: root
    anchors.margins: hifi.dimensions.contentMargin.x
    
    signal sendToScript(var message);

    color: hifi.colors.baseGray;

    property var config: Render.getConfig("RenderMainView.Fade");
    property var configEdit: Render.getConfig("RenderMainView.FadeEdit");

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.documents
        nameFilters: [ "JSON files (*.json)", "All files (*)" ]
        onAccepted: {
            root.sendToScript(title.split(" ")[0]+"*"+fileUrl.toString())
            // This is a hack to be sure the widgets below properly reflect the change of category: delete the Component
            // by setting the loader source to Null and then recreate it 500ms later
            paramWidgetLoader.sourceComponent = undefined;
            postpone.interval = 500
            postpone.start()
        }
        onRejected: {
        }
        Component.onCompleted: visible = false
    }

    ColumnLayout {
        spacing: 3
        anchors.left: parent.left
        anchors.right: parent.right      
        anchors.margins: hifi.dimensions.contentMargin.x  
        HifiControls.Label {
            text: "Transition"       
        }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true
            id: root_col

            HifiControls.ComboBox {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left : parent.left
                width: 300
                id: categoryBox
                model: ["Elements enter/leave domain", "Bubble isect. - Owner POV", "Bubble isect. - Trespasser POV", "Another user leaves/arrives", "Changing an avatar"]
                Timer {
                    id: postpone
                    interval: 100; running: false; repeat: false
                    onTriggered: { 
                        paramWidgetLoader.sourceComponent = paramWidgets
                        var isTimeBased = categoryBox.currentIndex==0 || categoryBox.currentIndex==3
                        paramWidgetLoader.item.isTimeBased = isTimeBased
                    }
                }
                onCurrentIndexChanged: {
                    var descriptions = [
                        "Time based threshold, gradients centered on object",
                        "Fixed threshold, gradients centered on owner avatar",
                        "Position based threshold (increases when trespasser moves closer to avatar), gradients centered on trespasser avatar",
                        "Time based threshold, gradients centered on bottom of object",
                        "UNSUPPORTED"
                    ]

                    description.text = descriptions[currentIndex]
                    root.config["editedCategory"] = currentIndex;
                    // This is a hack to be sure the widgets below properly reflect the change of category: delete the Component
                    // by setting the loader source to Null and then recreate it 100ms later
                    paramWidgetLoader.sourceComponent = undefined;
                    postpone.interval = 100
                    postpone.start()
                    root.sendToScript("category*"+currentIndex)
                }
            }
            HifiControls.Button {
                action: saveAction
                Layout.fillWidth: true
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }
            HifiControls.Button {
                action: loadAction
                Layout.fillWidth: true
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }
        }

        HifiControls.Label {
            id: description
            text: "..."
            Layout.fillWidth: true
            wrapMode: Text.WordWrap 
        }

        RowLayout {
            spacing: 20
            height: 36
            HifiControls.CheckBox {
                boxSize: 20
                anchors.verticalCenter: parent.verticalCenter
                text: "Manual"
                checked: root.config["manualFade"]
                onCheckedChanged: {
                    root.config["manualFade"] = checked;
                }
            }
            ConfigSlider {
                anchors.left: undefined
                anchors.verticalCenter: parent.verticalCenter
                height: 36
                width: 320
                label: "Threshold"
                integral: false
                config: root.config
                property: "manualThreshold"
                max: 1.0
                min: 0.0
            }
        }
        
        Action {
            id: saveAction
            text: "Save"
            onTriggered: {
                fileDialog.title = "Save configuration..."
                fileDialog.selectExisting = false
                fileDialog.open()
            }
        }
        Action {
            id: loadAction
            text: "Load"
            onTriggered: {
                fileDialog.title = "Load configuration..."
                fileDialog.selectExisting = true
                fileDialog.open()
            }
        }

        Separator {}  
              
        Component {
            id: paramWidgets

            ColumnLayout {
                spacing: 3
                width: root_col.width
                property bool isTimeBased

                RowLayout {
                    Layout.fillWidth: true

                    GroupBox {
                        title: "Base Gradient"
                        Layout.fillWidth: true

                        Column {
                            spacing: 3
                            anchors.left: parent.left
                            anchors.right: parent.right

                            Repeater {
                                model: [
                                "Size X:baseSizeX", 
                                "Size Y:baseSizeY", 
                                "Size Z:baseSizeZ",
                                "Level:baseLevel" ]                  

                                ConfigSlider {
                                    height: 36
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
                        Layout.fillWidth: true
                        
                        Column {
                            spacing: 3
                            anchors.left: parent.left
                            anchors.right: parent.right

                            Repeater {
                                model: [
                                "Size X:noiseSizeX", 
                                "Size Y:noiseSizeY", 
                                "Size Z:noiseSizeZ",
                                "Level:noiseLevel" ]                  

                                ConfigSlider {
                                    height: 36
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
                }

                RowLayout {
                    spacing: 20
                    height: 36

                    HifiControls.CheckBox {
                        text: "Invert gradient"
                        anchors.verticalCenter: parent.verticalCenter
                        boxSize: 20
                        checked: root.config["isInverted"]
                        onCheckedChanged: { root.config["isInverted"] = checked }
                    }
                    ConfigSlider {
                        anchors.left: undefined
                        anchors.verticalCenter: parent.verticalCenter
                        height: 36
                        width: 300
                        label: "Edge Width"
                        integral: false
                        config: root.config
                        property: "edgeWidth"
                        max: 1.0
                        min: 0.0
                    }
                }


                RowLayout {
                    Layout.fillWidth: true

                    GroupBox {
                        title: "Edge Inner color"
                        Layout.fillWidth: true

                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right

                            Color {
                                height: 30
                                anchors.left: parent.left
                                anchors.right: parent.right
                                _color: root.config.edgeInnerColor
                                onNewColor: {
                                    root.config.edgeInnerColor = _color
                                }
                            }
                            ConfigSlider {
                                height: 36
                                label: "Intensity"
                                integral: false
                                config: root.config
                                property: "edgeInnerIntensity"
                                max: 1.0
                                min: 0.0
                            }
                        }
                    }
                    GroupBox {
                        title: "Edge Outer color"
                        Layout.fillWidth: true

                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right 

                            Color {
                                height: 30
                                anchors.left: parent.left
                                anchors.right: parent.right
                                _color: root.config.edgeOuterColor
                                onNewColor: {
                                    root.config.edgeOuterColor = _color
                                }
                            }
                            ConfigSlider {
                                height: 36
                                label: "Intensity"
                                integral: false
                                config: root.config
                                property: "edgeOuterIntensity"
                                max: 1.0
                                min: 0.0
                            }
                        }
                    }
                }
                GroupBox {
                    title: "Timing"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right 

                        RowLayout {
                            Layout.fillWidth: true

                            ConfigSlider {
                                enabled: isTimeBased
                                opacity: isTimeBased ? 1.0 : 0.0
                                anchors.left: undefined
                                anchors.right: undefined
                                Layout.fillWidth: true
                                height: 36
                                label: "Duration"
                                integral: false
                                config: root.config
                                property: "duration"
                                max: 10.0
                                min: 0.1
                            }
                            HifiControls.ComboBox {
                                enabled: isTimeBased
                                opacity: isTimeBased ? 1.0 : 0.0
                                Layout.fillWidth: true
                                model: ["Linear", "Ease In", "Ease Out", "Ease In / Out"]
                                currentIndex: root.config["timing"]
                                onCurrentIndexChanged: {
                                    root.config["timing"] = currentIndex;
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true

                            function evalEvenHeight() {
                                // Why do we have to do that manually ? cannot seem to find a qml / anchor / layout mode that does that ?
                                return (height - spacing * (children.length - 1)) / children.length
                            }

                            GroupBox {
                                title: "Noise Animation"
                                Layout.fillWidth: true
                                id: animBox

                                Column {
                                    anchors.left: parent.left
                                    anchors.right: parent.right 

                                    Repeater {
                                        model: [
                                        "Speed X:noiseSpeedX", 
                                        "Speed Y:noiseSpeedY", 
                                        "Speed Z:noiseSpeedZ" ]                  

                                        ConfigSlider {
                                            height: 36
                                            label:  modelData.split(":")[0]
                                            integral: false
                                            config: root.config
                                            property:  modelData.split(":")[1]
                                            max: 1.0
                                            min: -1.0
                                        }
                                    }
                                }
                            }

                            PlotPerf {
                                title: "Threshold"
                                width: 200
                                anchors.top: animBox.top
                                anchors.bottom : animBox.bottom
                                object: root.config
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
        }



        Loader {
            id: paramWidgetLoader
            sourceComponent: paramWidgets
        }
        
    }
}