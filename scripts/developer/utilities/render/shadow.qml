//
//  shadow.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 10/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
    
import "configSlider"

Rectangle {
    id: root;

    HifiConstants { id: hifi; }
    color: hifi.colors.baseGray;

    property var viewConfig: Render.getConfig("RenderMainView.DrawViewFrustum");
    property var shadowConfig : Render.getConfig("RenderMainView.ShadowSetup");
    property var shadow0Config: Render.getConfig("RenderMainView.DrawShadowFrustum0");
    property var shadow1Config: Render.getConfig("RenderMainView.DrawShadowFrustum1");
    property var shadow2Config: Render.getConfig("RenderMainView.DrawShadowFrustum2");
    property var shadow3Config: Render.getConfig("RenderMainView.DrawShadowFrustum3");
    property var shadowBBox0Config: Render.getConfig("RenderMainView.DrawShadowBBox0");
    property var shadowBBox1Config: Render.getConfig("RenderMainView.DrawShadowBBox1");
    property var shadowBBox2Config: Render.getConfig("RenderMainView.DrawShadowBBox2");
    property var shadowBBox3Config: Render.getConfig("RenderMainView.DrawShadowBBox3");

    Component.onCompleted: {
        viewConfig.enabled = true;
        shadow0Config.enabled = true;
        shadow1Config.enabled = true;
        shadow2Config.enabled = true;
        shadow3Config.enabled = true;
        shadowBBox0Config.enabled = true;
        shadowBBox1Config.enabled = true;
        shadowBBox2Config.enabled = true;
        shadowBBox3Config.enabled = true;
    }
    Component.onDestruction: {
        viewConfig.enabled = false;
        shadow0Config.enabled = false;
        shadow1Config.enabled = false;
        shadow2Config.enabled = false;
        shadow3Config.enabled = false;
        shadow0Config.isFrozen = false;
        shadow1Config.isFrozen = false;
        shadow2Config.isFrozen = false;
        shadow3Config.isFrozen = false;

        shadowBBox0Config.enabled = false;
        shadowBBox1Config.enabled = false;
        shadowBBox2Config.enabled = false;
        shadowBBox3Config.enabled = false;
        shadowBBox0Config.isFrozen = false;
        shadowBBox1Config.isFrozen = false;
        shadowBBox2Config.isFrozen = false;
        shadowBBox3Config.isFrozen = false;
    }

    ColumnLayout {
        spacing: 10
        anchors.left: parent.left
        anchors.right: parent.right      
        anchors.margins: hifi.dimensions.contentMargin.x  

        RowLayout {
            spacing: 20
            Layout.fillWidth: true

            HifiControls.CheckBox {
                boxSize: 20
                text: "Freeze"
                checked: false
                onCheckedChanged: { 
                    viewConfig.isFrozen = checked;
                    shadow0Config.isFrozen = checked;
                    shadow1Config.isFrozen = checked;
                    shadow2Config.isFrozen = checked;
                    shadow3Config.isFrozen = checked;

                    shadowBBox0Config.isFrozen = checked;
                    shadowBBox1Config.isFrozen = checked;
                    shadowBBox2Config.isFrozen = checked;
                    shadowBBox3Config.isFrozen = checked;
                }
            }
            HifiControls.Label {
                text: "View"
                color: "yellow"
                font.italic: true
            }
            HifiControls.Label {
                text: "Shadow"
                color: "blue"
                font.italic: true
            }
            HifiControls.Label {
                text: "AABB"
                color: "red"
                font.italic: true
            }
        }
        ConfigSlider {
            label: qsTr("Bias Input")
            integral: false
            config: shadowConfig
            property: "biasInput"
            max: 1.0
            min: 0.0
            height: 38
            width: 250
        }
        ConfigSlider {
            label: qsTr("Shadow Max Distance")
            integral: false
            config: shadowConfig
            property: "maxDistance"
            max: 250.0
            min: 0.0
            height: 38
            width: 250
        }
        Repeater {
            model: [
                "0", "1", "2", "3"
            ]
            ColumnLayout {
                spacing: 8
                anchors.left: parent.left
                anchors.right: parent.right      

                HifiControls.Separator {
                    anchors.left: parent.left
                    anchors.right: parent.right
                }   
                HifiControls.CheckBox {
                    text: "Cascade "+modelData
                    boxSize: 20
                    checked: Render.getConfig("RenderMainView.DrawShadowFrustum"+modelData)
                    onCheckedChanged: { 
                        Render.getConfig("RenderMainView.DrawShadowFrustum"+modelData).enabled = checked;
                        Render.getConfig("RenderMainView.DrawShadowBBox"+modelData).enabled = checked;
                    }
                }
                ConfigSlider {
                    label: qsTr("Constant bias")
                    integral: false
                    config: shadowConfig
                    property: "constantBias"+modelData
                    max: 1.0
                    min: 0.0
                    height: 38
                    width: 250
                }
                ConfigSlider {
                    label: qsTr("Slope bias")
                    integral: false
                    config: shadowConfig
                    property: "slopeBias"+modelData
                    max: 1.0
                    min: 0.0
                    height: 38
                    width: 250
                }
            }
        }
    }
}
