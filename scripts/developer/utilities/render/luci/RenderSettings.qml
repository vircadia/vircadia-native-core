//
//  Render Settings.qml
//
//  Created by Sam Gateau on 5/28/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import controlsUit 1.0 as HifiControls

import "../../lib/prop" as Prop

Column {
    anchors.fill: parent 
    id: theGrapchicsSettings;   

    Prop.PropEnum {
        label: "Render Method"
        object: Render
        property: "renderMethod"
        enums: [
            "Deferred",
            "Forward"
                ]
    }

    Prop.PropBool {
        label: "Shadows"
        object: Render
        property: "shadowsEnabled"  
    }
}

