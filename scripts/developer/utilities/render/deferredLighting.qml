//
//  deferredLighting.qml
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"

Column {
    spacing: 8
    Column {
        id: deferredLighting
        spacing: 10

        CheckBox {
            text: "Point Lights"
            checked: true
            onCheckedChanged: { Render.getConfig("RenderDeferred").enablePointLights = checked }
        }
        CheckBox {
            text: "Spot Lights"
            checked: true
            onCheckedChanged: { Render.getConfig("RenderDeferred").enableSpotLights = checked }
        }
    }
}
