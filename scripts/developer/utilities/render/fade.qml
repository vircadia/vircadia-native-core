//
//  fade.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 30/04/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"

Column {
    id: root
    spacing: 8
    property var drawOpaqueConfig: Render.getConfig("DrawOpaqueDeferred");

	CheckBox {
		text: "Force Fade"
		checked: drawOpaqueConfig["debugFade"]
		onCheckedChanged: { drawOpaqueConfig["debugFade"] = checked }
	}
	ConfigSlider {
		label: "Percent"
		integral: false
		config: drawOpaqueConfig
		property: "debugFadePercent"
		max: 1.0
		min: 0.0
	}
}
