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
import PerformanceEnums 1.0

Item {
    HifiStylesUit.HifiConstants { id: hifi; }

    id: root;
    anchors.fill: parent

    ColumnLayout {
        id: graphicsSettingsColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        anchors.topMargin: HMD.active ? 80 : 0
        spacing: 8

        ColumnLayout {
            Layout.preferredWidth: parent.width
            Layout.topMargin: 18
            spacing: 0

            HifiStylesUit.RalewayRegular {
                text: "GRAPHICS SETTINGS"
                Layout.maximumWidth: parent.width
                height: 30
                size: 16
                color: "#FFFFFF"
            }

            ColumnLayout {
                Layout.topMargin: 10
                Layout.preferredWidth: parent.width
                spacing: 0

                HifiControlsUit.RadioButton {
                    id: performanceLowPower
                    colorScheme: hifi.colorSchemes.dark
                    height: 18
                    fontSize: 16
                    leftPadding: 0
                    text: "Low Power"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.LOW_POWER
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.LOW_POWER);
                        root.refreshAllDropdowns();
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceLow
                    colorScheme: hifi.colorSchemes.dark
                    height: 18
                    fontSize: 16
                    leftPadding: 0
                    text: "Low"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.LOW
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.LOW);
                        root.refreshAllDropdowns();
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceMedium
                    colorScheme: hifi.colorSchemes.dark
                    height: 18
                    fontSize: 16
                    leftPadding: 0
                    text: "Medium"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.MID
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.MID);
                        root.refreshAllDropdowns();
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceHigh
                    colorScheme: hifi.colorSchemes.dark
                    height: 18
                    fontSize: 16
                    leftPadding: 0
                    text: "High"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.HIGH
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.HIGH);
                        root.refreshAllDropdowns();
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceCustom
                    colorScheme: hifi.colorSchemes.dark
                    height: 18
                    fontSize: 16
                    leftPadding: 0
                    text: "Custom"
                    checked: Performance.getPerformancePreset() === PerformanceEnums.UNKNOWN
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.UNKNOWN);
                    }
                }
            }

            ColumnLayout {
                Layout.topMargin: 10
                Layout.preferredWidth: parent.width
                spacing: 0

                Item {
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 35

                    HifiStylesUit.RalewayRegular {
                        id: worldDetailHeader
                        text: "World Detail"
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 130
                        height: parent.height
                        size: 16
                        color: "#FFFFFF"
                    }

                    ListModel {
                        id: worldDetailModel

                        ListElement {
                            text: "Low World Detail"
                        }
                        ListElement {
                            text: "Medium World Detail"
                        }
                        ListElement {
                            text: "Full World Detail"
                        }
                    }
                
                    HifiControlsUit.ComboBox {
                        id: worldDetailDropdown
                        enabled: performanceCustom.checked
                        anchors.left: worldDetailHeader.right
                        anchors.leftMargin: 20
                        anchors.top: parent.top
                        width: 280
                        height: parent.height
                        colorScheme: hifi.colorSchemes.dark
                        model: worldDetailModel
                        currentIndex: -1

                        function refreshWorldDetailDropdown() {
                            worldDetailDropdown.currentIndex = LODManager.worldDetailQuality;
                        }

                        Component.onCompleted: {
                            worldDetailDropdown.refreshWorldDetailDropdown();
                        }
                        
                        onCurrentIndexChanged: {
                            LODManager.worldDetailQuality = currentIndex;
                            worldDetailDropdown.displayText = model.get(currentIndex).text;
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: parent.width
                    Layout.topMargin: 20

                    HifiStylesUit.RalewayRegular {
                        id: renderingEffectsHeader
                        text: "Rendering Effects"
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 130
                        height: parent.height
                        size: 16
                        color: "#FFFFFF"
                    }

                    ColumnLayout {
                        anchors.left: renderingEffectsHeader.right
                        anchors.leftMargin: 20
			Layout.preferredWidth: parent.width
			spacing: 0
                        enabled: performanceCustom.checked

                        HifiControlsUit.RadioButton {
                            id: renderingEffectsDisabled
                            colorScheme: hifi.colorSchemes.dark
                            height: 18
                            fontSize: 16
                            leftPadding: 0
                            text: "Disabled"
                            checked: Render.renderMethod === 1
                            onClicked: {
                                Render.renderMethod = 1; // "FORWARD"
                                //refreshRenderingEffectCheckboxes();
                            }
                        }

                        HifiControlsUit.RadioButton {
                            id: renderingEffectsEnabled
                            enabled: PlatformInfo.isRenderMethodDeferredCapable()
                            colorScheme: hifi.colorSchemes.dark
                            height: 18
                            fontSize: 16
                            leftPadding: 0
                            text: "Enabled"
                            checked: Render.renderMethod === 0
                            onClicked: {
                                Render.renderMethod = 0; // "DEFERRED"
                            }
                        }

                        ColumnLayout {
                            id: renderingEffectCheckboxes
                            Layout.preferredWidth: parent.width
                            anchors.left: parent.left
                            anchors.leftMargin: 24
                            anchors.topMargin: 8
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: Layout.topMargin
                            enabled: performanceCustom.checked && renderingEffectsEnabled.checked

                            HifiControlsUit.CheckBox {
                                id: renderingEffectShadows
                                checked: Render.shadowsEnabled
                                boxSize: 16
                                text: "Shadows"
                                spacing: -1
                                colorScheme: hifi.colorSchemes.dark
                                anchors.left: parent.left
                                anchors.top: parent.top
                                onCheckedChanged: {
                                    Render.shadowsEnabled = renderingEffectShadows.checked;
                                }
                            }
                            HifiControlsUit.CheckBox {
                                id: renderingEffectLocalLights
                                enabled: false
                                //checked: Render.localLightsEnabled
                                checked: renderingEffectsEnabled.checked
                                boxSize: 16
                                text: "Local lights"
                                spacing: -1
                                colorScheme: hifi.colorSchemes.dark
                                anchors.left: parent.left
                                anchors.top: renderingEffectShadows.bottom
                                //onCheckedChanged: {
                                //    Render.localLightsEnabled = renderingEffectLocalLightsEnabled.checked;
                                //}
                            }
                            HifiControlsUit.CheckBox {
                                id: renderingEffectFog
                                enabled: false
                                //checked: Render.fogEnabled
                                checked: renderingEffectsEnabled.checked
                                boxSize: 16
                                text: "Fog"
                                spacing: -1
                                colorScheme: hifi.colorSchemes.dark
                                anchors.left: parent.left
                                anchors.top: renderingEffectLocalLights.bottom
                                //onCheckedChanged: {
                                //    Render.fogEnabled = renderingEffectFogEnabled.checked;
                                //}
                            }
                            HifiControlsUit.CheckBox {
                                id: renderingEffectBloom
                                enabled: false
                                //checked: Render.bloomEnabled
                                checked: renderingEffectsEnabled.checked
                                boxSize: 16
                                text: "Bloom"
                                spacing: -1
                                colorScheme: hifi.colorSchemes.dark
                                anchors.left: parent.left
                                anchors.top: renderingEffectFog.bottom
                                //onCheckedChanged: {
                                //    Render.bloomEnabled = renderingEffectBloomEnabled.checked;
                                //}
                            }
                        }
                    }
                }

                Item {
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 35
                    Layout.topMargin: 10

                    HifiStylesUit.RalewayRegular {
                        id: refreshRateHeader
                        text: "Refresh Rate"
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 130
                        height: parent.height
                        size: 16
                        color: "#FFFFFF"
                    }

                    ListModel {
                        id: refreshRateModel

                        ListElement {
                            text: "Economical"
                            refreshRatePreset: 0 // RefreshRateProfile::ECO
                        }
                        ListElement {
                            text: "Interactive"
                            refreshRatePreset: 1 // RefreshRateProfile::INTERACTIVE
                        }
                        ListElement {
                            text: "Real-Time"
                            refreshRatePreset: 2 // RefreshRateProfile::REALTIME
                        }
                    }
                
                    HifiControlsUit.ComboBox {
                        id: refreshRateDropdown
                        enabled: performanceCustom.checked
                        anchors.left: refreshRateHeader.right
                        anchors.leftMargin: 20
                        anchors.top: parent.top
                        width: 280
                        height: parent.height
                        colorScheme: hifi.colorSchemes.dark
                        model: refreshRateModel
                        currentIndex: -1

                        function refreshRefreshRateDropdownDisplay() {
                            if (Performance.getRefreshRateProfile() === 0) {
                                refreshRateDropdown.currentIndex = 0;
                            } else if (Performance.getRefreshRateProfile() === 1) {
                                refreshRateDropdown.currentIndex = 1;
                            } else {
                                refreshRateDropdown.currentIndex = 2;
                            }
                        }

                        Component.onCompleted: {
                            refreshRateDropdown.refreshRefreshRateDropdownDisplay();
                        }
                        
                        onCurrentIndexChanged: {
                            Performance.setRefreshRateProfile(model.get(currentIndex).refreshRatePreset);
                            refreshRateDropdown.displayText = model.get(currentIndex).text;
                        }
                    }
                }

                Item {
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 35
                    Layout.topMargin: 16

                    HifiStylesUit.RalewayRegular {
                        id: resolutionHeader
                        text: "Resolution Scale (" + Number.parseFloat(Render.viewportResolutionScale).toPrecision(3) + ")"
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 130
                        height: parent.height
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
                        height: parent.height
                        colorScheme: hifi.colorSchemes.dark
                        minimumValue: 0.25
                        maximumValue: 2.0
                        stepSize: 0.05
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

            ColumnLayout {
                Layout.topMargin: 20
                Layout.preferredWidth: parent.width
                spacing: 0
    
                Item {
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 35
    
                    HifiStylesUit.RalewayRegular {
                        id: antialiasingHeader
                        text: "Anti-aliasing"
                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 130
                        height: parent.height
                        size: 16
                        color: "#FFFFFF"
                    }

                    ListModel {
                        id: antialiasingModel

                        // Maintain same order as "AntialiasingConfig::Mode".
                        ListElement {
                            text: "None"
                        }
                        ListElement {
                            text: "TAA"
                        }
                        ListElement {
                            text: "FXAA"
                        }
                    }
    
                    HifiControlsUit.ComboBox {
                        id: antialiasingDropdown
                        anchors.left: antialiasingHeader.right
                        anchors.leftMargin: 20
                        anchors.top: parent.top
                        width: 280
                        height: parent.height
                        colorScheme: hifi.colorSchemes.dark
                        model: antialiasingModel
                        currentIndex: -1
    
                        function refreshAntialiasingDropdown() {
                            antialiasingDropdown.currentIndex = Render.antialiasingMode;
                        }
    
                        Component.onCompleted: {
                            antialiasingDropdown.refreshAntialiasingDropdown();
                        }
    
                        onCurrentIndexChanged: {
                            Render.antialiasingMode = currentIndex;
                            antialiasingDropdown.displayText = model.get(currentIndex).text;
                        }
                    }
                }
            }
        }
    }

    function refreshAllDropdowns() {
        worldDetailDropdown.refreshWorldDetailDropdown();
        refreshRateDropdown.refreshRefreshRateDropdownDisplay();
        antialiasingDropdown.refreshAntialiasingDropdown();
    }
}
