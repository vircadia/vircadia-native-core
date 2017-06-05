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

Row {
	property var drawOpaqueConfig: Render.getConfig("DrawOpaqueDeferred");
	property var drawTransparentConfig: Render.getConfig("DrawTransparentDeferred");
	spacing: 4
	Column {
		spacing: 8

		CheckBox {
			text: "Force Fade Opaque"
			checked: drawOpaqueConfig["debugFade"]
			onCheckedChanged: { drawOpaqueConfig["debugFade"] = checked }
		}
		CheckBox {
			text: "Force Fade Transparent"
			checked: drawTransparentConfig["debugFade"]
			onCheckedChanged: { drawTransparentConfig["debugFade"] = checked }
		}
	}
	Column {
		spacing: 8

		ConfigSlider {
			label: "Percent"
			integral: false
			config: drawOpaqueConfig
			property: "debugFadePercent"
			max: 1.0
			min: 0.0
			width: 250
		}
		ConfigSlider {
			label: "Percent"
			integral: false
			config: drawTransparentConfig
			property: "debugFadePercent"
			max: 1.0
			min: 0.0
			width: 250
		}
	}
}

