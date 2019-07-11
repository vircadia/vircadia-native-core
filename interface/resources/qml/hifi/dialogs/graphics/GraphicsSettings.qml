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
                Layout.preferredHeight: contentItem.height
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
                        root.refreshAllDropdowns();
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
                        root.refreshAllDropdowns();
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
                        root.refreshAllDropdowns();
                    }
                }

                HifiControlsUit.RadioButton {
                    id: performanceCustom
                    colorScheme: hifi.colorSchemes.dark
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
                Layout.preferredHeight: contentItem.height
                spacing: 30

                Item {
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 35

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
                            worldDetailQualityValue: 0.25
                        }
                        ListElement {
                            text: "Medium World Detail"
                            worldDetailQualityValue: 0.5
                        }
                        ListElement {
                            text: "Full World Detail"
                            worldDetailQualityValue: 0.75
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
                            var currentWorldDetailQuality = LODManager.worldDetailQuality;
                            if (currentWorldDetailQuality <= 0.25) {
                                worldDetailDropdown.currentIndex = 0;
                            } else if (currentWorldDetailQuality <= 0.5) {
                                worldDetailDropdown.currentIndex = 1;
                            } else {
                                worldDetailDropdown.currentIndex = 2;
                            }
                        }

                        Component.onCompleted: {
                            worldDetailDropdown.refreshWorldDetailDropdown();
                        }
                        
                        onCurrentIndexChanged: {
                            LODManager.worldDetailQuality = model.get(currentIndex).worldDetailQualityValue;
                            worldDetailDropdown.displayText = model.get(currentIndex).text;
                        }
                    }
                }

                Item {
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 35

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

                    ListModel {
                        id: renderingEffectsModel

                        ListElement {
                            text: "No Rendering Effects"
                            preferredRenderMethod: 1 // "FORWARD"
                            shadowsEnabled: false
                        }
                        ListElement {
                            text: "Local Lights, Fog, Bloom"
                            preferredRenderMethod: 0 // "DEFERRED"
                            shadowsEnabled: false
                        }
                        ListElement {
                            text: "Local Lights, Fog, Bloom, Shadows"
                            preferredRenderMethod: 0 // "DEFERRED"
                            shadowsEnabled: true
                        }
                    }
                
                    HifiControlsUit.ComboBox {
                        id: renderingEffectsDropdown
                        enabled: performanceCustom.checked
                        anchors.left: renderingEffectsHeader.right
                        anchors.leftMargin: 20
                        anchors.top: parent.top
                        width: 280
                        height: parent.height
                        colorScheme: hifi.colorSchemes.dark
                        model: renderingEffectsModel
                        currentIndex: -1

                        function refreshRenderingEffectsDropdownDisplay() {
                            if (Render.shadowsEnabled) {
                                renderingEffectsDropdown.currentIndex = 2;
                            } else if (Render.renderMethod === 0) {
                                renderingEffectsDropdown.currentIndex = 1;
                            } else {
                                renderingEffectsDropdown.currentIndex = 0;
                            }
                        }

                        Component.onCompleted: {
                            renderingEffectsDropdown.refreshRenderingEffectsDropdownDisplay();
                        }
                        
                        onCurrentIndexChanged: {
                            var renderMethodToSet = 1;
                            if (model.get(currentIndex).preferredRenderMethod === 0 &&
                                PlatformInfo.isRenderMethodDeferredCapable()) {
                                renderMethodToSet = 0;
                            }
                            Render.renderMethod = model.get(currentIndex).preferredRenderMethod;
                            Render.shadowsEnabled = model.get(currentIndex).shadowsEnabled;
                            renderingEffectsDropdown.displayText = model.get(currentIndex).text;
                        }
                    }
                }

                Item {
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 35

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
            }
        }
    }

    function refreshAllDropdowns() {
        worldDetailDropdown.refreshWorldDetailDropdown();
        renderingEffectsDropdown.refreshRenderingEffectsDropdownDisplay();
        refreshRateDropdown.refreshRefreshRateDropdownDisplay();
    }
}
