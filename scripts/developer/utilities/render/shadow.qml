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
import QtQuick 2.5
import QtQuick.Controls 1.4
import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls
import  "configSlider"

Column {
    id: root
    spacing: 8
    property var viewConfig: Render.getConfig("RenderMainView.DrawViewFrustum");
    property var shadowConfig : Render.getConfig("RenderMainView.ShadowSetup");
    property var shadow0Config: Render.getConfig("RenderMainView.DrawShadowFrustum0");
    property var shadow1Config: Render.getConfig("RenderMainView.DrawShadowFrustum1");
    property var shadow2Config: Render.getConfig("RenderMainView.DrawShadowFrustum2");
    property var shadow3Config: Render.getConfig("RenderMainView.DrawShadowFrustum3");

    Component.onCompleted: {
        viewConfig.enabled = true;
        shadow0Config.enabled = true;
        shadow1Config.enabled = true;
        shadow2Config.enabled = true;
        shadow3Config.enabled = true;
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
        shadow0BoundConfig.isFrozen = false;
        shadow1BoundConfig.isFrozen = false;
        shadow2BoundConfig.isFrozen = false;
        shadow3BoundConfig.isFrozen = false;
    }

    CheckBox {
        text: "Freeze Frustums"
        checked: false
        onCheckedChanged: { 
            viewConfig.isFrozen = checked;
            shadow0Config.isFrozen = checked;
            shadow1Config.isFrozen = checked;
            shadow2Config.isFrozen = checked;
            shadow3Config.isFrozen = checked;
            shadow0BoundConfig.isFrozen = checked;
            shadow1BoundConfig.isFrozen = checked;
            shadow2BoundConfig.isFrozen = checked;
            shadow3BoundConfig.isFrozen = checked;
        }
    }
    Row {
        spacing: 8
        Label {
            text: "View"
            color: "yellow"
            font.italic: true
        }
        Label {
            text: "Shadow"
            color: "blue"
            font.italic: true
        }
        Label {
            text: "Items"
            color: "magenta"
            font.italic: true
        }
    }
    ConfigSlider {
        label: qsTr("Cascade 0 constant bias")
        integral: false
        config: shadowConfig
        property: "constantBias0"
        max: 1.0
        min: 0.0
    }
    ConfigSlider {
        label: qsTr("Cascade 1 constant bias")
        integral: false
        config: shadowConfig
        property: "constantBias1"
        max: 1.0
        min: 0.0
    }
    ConfigSlider {
        label: qsTr("Cascade 2 constant bias")
        integral: false
        config: shadowConfig
        property: "constantBias2"
        max: 1.0
        min: 0.0
    }
    ConfigSlider {
        label: qsTr("Cascade 3 constant bias")
        integral: false
        config: shadowConfig
        property: "constantBias3"
        max: 1.0
        min: 0.0
    }

    ConfigSlider {
        label: qsTr("Cascade 0 slope bias")
        integral: false
        config: shadowConfig
        property: "slopeBias0"
        max: 1.0
        min: 0.0
    }
    ConfigSlider {
        label: qsTr("Cascade 1 slope bias")
        integral: false
        config: shadowConfig
        property: "slopeBias1"
        max: 1.0
        min: 0.0
    }
    ConfigSlider {
        label: qsTr("Cascade 2 slope bias")
        integral: false
        config: shadowConfig
        property: "slopeBias2"
        max: 1.0
        min: 0.0
    }
    ConfigSlider {
        label: qsTr("Cascade 3 slope bias")
        integral: false
        config: shadowConfig
        property: "slopeBias3"
        max: 1.0
        min: 0.0
    }
}
