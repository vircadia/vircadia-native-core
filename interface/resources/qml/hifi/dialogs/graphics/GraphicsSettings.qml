//
//  GraphicsSettings.qml
//  qml\hifi\dialogs\graphics
//
//  Created by Zach Fox on 2019-07-10
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.12
import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HifiControlsUit
import "qrc:////qml//controls" as HifiControls

Flickable {
    HifiStylesUit.HifiConstants { id: hifi; }

    id: root;
    contentWidth: parent.width
    contentHeight: graphicsSettingsColumnLayout.height
    clip: true

    ColumnLayout {
        id: graphicsSettingsColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: 8

        ColumnLayout {
            id: avatarNameTagsContainer
            Layout.preferredWidth: parent.width
            Layout.topMargin: 38
            spacing: 0

            HifiStylesUit.RalewayRegular {
                text: "GRAPHICS SETTINGS"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: "#FFFFFF"
            }

            ColumnLayout {
                Layout.topMargin: 10
                Layout.preferredWidth: parent.width
                spacing: 0

                HifiControlsUit.RadioButton {
                    id: performanceLow
                    colorScheme: hifi.colorSchemes.dark
                    fontSize: 16
                    leftPadding: 0
                    text: "Low"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.LOW
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.LOW);
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceMedium
                    colorScheme: hifi.colorSchemes.dark
                    fontSize: 16
                    leftPadding: 0
                    text: "Medium"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.MID
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.MID);
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceHigh
                    colorScheme: hifi.colorSchemes.dark
                    fontSize: 16
                    leftPadding: 0
                    text: "High"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.HIGH
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.HIGH);
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceCustom
                    colorScheme: hifi.colorSchemes.dark
                    fontSize: 16
                    leftPadding: 0
                    text: "Custom"
                    checked: !(performanceLow.checked || performanceMedium.checked || performanceHigh.checked) 
                    onClicked: {
                        
                    }
                }
            }

            ColumnLayout {
                Layout.topMargin: 10
                Layout.preferredWidth: parent.width
                spacing: 0

                Item {
                    Layout.preferredWidth: parent.width

                    HifiStylesUit.RalewayRegular {
                        id: resolutionHeader
                        text: "Resolution Scale (" + Number.parseFloat(Render.viewportResolutionScale).toPrecision(3) + ")"
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 130
                        height: paintedHeight
                        size: 16
                        color: "#FFFFFF"
                    }
                
                    HifiControlsUit.Slider {
                        id: resolutionScaleSlider
                        enabled: performanceCustom.checked
                        anchors.left: resolutionHeader.right
                        anchors.leftMargin: 57
                        anchors.top: parent.top
                        width: 150 
                        height: resolutionHeader.height
                        colorScheme: hifi.colorSchemes.dark
                        minimumValue: 0.25
                        maximumValue: 1.5
                        stepSize: 0.02
                        value: Render.viewportResolutionScale
                        live: true

                        function updateResolutionScale(sliderValue) {
                            if (Render.viewportResolutionScale !== sliderValue) {
                                Render.viewportResolutionScale = sliderValue;
                            }
                        }

                        onValueChanged: {
                            updateResolutionScale(value);
                        }
                        onPressedChanged: {
                            if (!pressed) {
                                updateResolutionScale(value);
                            }
                        }
                    }
                }
            }
        }
    }
}
