//
//  HiFiConstants.qml
//
//  Created by Alexander Ivash on 17 Apr 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Window 2.2
import stylesUit 1.0

QtObject {
	readonly property QtObject colors: QtObject {
        readonly property color lightGrayBackground: "#f2f2f2"
		readonly property color black: "#000000"
		readonly property color white: "#ffffff"
		readonly property color blueHighlight: "#00b4ef"
		readonly property color inputFieldBackground: "#d4d4d4"
		readonly property color yellowishOrange: "#ffb017"
		readonly property color blueAccent: "#0093c5"
		readonly property color greenHighlight: "#1fc6a6"
		readonly property color lightGray: "#afafaf"
		readonly property color redHighlight: "#ea4c5f"
		readonly property color orangeAccent: "#ff6309"
	}
}
