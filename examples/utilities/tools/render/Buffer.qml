//
//  Buffer.qml
//  examples/utilities/tools/render
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    width: 200
    height: 270

    Label {
        text: qsTr("Debug Buffer")
    }

    ExclusiveGroup { id: buffer }

    function setDebugMode(mode) {
        var debug = Render.getConfig("DebugDeferredBuffer");
        console.log(mode);
        debug.enabled = (mode != 0);
        debug.mode = mode;
    }

    RadioButton {
        x: 8; y: 19 + 0 * 23
        text: qsTr("Off")
        exclusiveGroup: buffer
        checked: true
        onCheckedChanged: { if (checked) { setDebugMode(0) } }
    }
    RadioButton {
        x: 8; y: 19 + 1 * 23
        text: qsTr("Diffuse")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(1) } }
    }
    RadioButton {
        x: 8; y: 19 + 2 * 23
        text: qsTr("Metallic")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(2) } }
    }
    RadioButton {
        x: 8; y: 19 + 3 * 23
        text: qsTr("Roughness")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(3) } }
    }
    RadioButton {
        x: 8; y: 19 + 4 * 23
        text: qsTr("Normal") 
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(4) } }
    }
    RadioButton {
        x: 8; y: 19 + 5 * 23
        text: qsTr("Depth")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(5) } }
    }
    RadioButton {
        x: 8; y: 19 + 6 * 23
        text: qsTr("Lighting")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(6) } }
    }
    RadioButton {
        x: 8; y: 19 + 7 * 23
        text: qsTr("Shadow") 
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(7) } }
    }
    RadioButton {
        x: 8; y: 19 + 8 * 23
        text: qsTr("Pyramid Depth")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(8) } }
    }
    RadioButton {
        x: 8; y: 19 + 9 * 23
        text: qsTr("Ambient Occlusion")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(9) } }
    }
    RadioButton {
        x: 8; y: 19 + 10 * 23
        text: qsTr("Custom Shader")
        exclusiveGroup: buffer
        onCheckedChanged: { if (checked) { setDebugMode(10) } }
    }
}

