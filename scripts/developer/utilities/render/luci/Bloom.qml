//
//  bloom.qml
//
//  Olivier Prat, created on 09/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import "../../lib/prop" as Prop

Column {
    anchors.left: parent.left
    anchors.right: parent.right 

    id: bloom

    property var config: Render.getConfig("RenderMainView.DebugBloom")

    Prop.PropBool {
        label: "Apply Bloom"
        object: Render.getConfig("RenderMainView.LightingModel")
        property: "enableBloom"
    } 
    
    function setDebugMode(mode) {
        console.log("Bloom mode is " + mode)
        bloom.config.enabled = (mode != 0);
        bloom.config.mode = mode;
    }

    Prop.PropEnum {
        label: "Debug Bloom Buffer"
       // object: config
       // property: "mode"
        enums: [
            "Off",
            "Lvl 0",
            "Lvl 1",
            "Lvl 2",
            "All",
                ]

        valueVarSetter: function (mode) { bloom.setDebugMode(mode) }
    }
}

