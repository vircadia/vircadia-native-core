//
//  HiFiConstants.qml
//
//  Created by Bradley Austin Davis on 28 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Window 2.2

Item {
    readonly property alias colors: colors
    readonly property alias dimensions: dimensions
    readonly property alias effects: effects

    Item {
        id: colors
        readonly property color white: "#ffffff"
        readonly property color baseGray: "#404040"
        readonly property color darkGray: "#121212"
        readonly property color baseGrayShadow: "#252525"
        readonly property color baseGrayHighlight: "#575757"
        readonly property color lightGray: "#6a6a6a"
        readonly property color lightGrayText: "#afafaf"
        readonly property color faintGray: "#e3e3e3"
        readonly property color primaryHighlight: "#00b4ef"
        readonly property color blueHighlight: "#00b4ef"
        readonly property color blueAccent: "#1080b8"
        readonly property color redHighlight: "#e2334d"
        readonly property color redAccent: "#b70a37"
        readonly property color greenHighlight: "#1ac567"
        readonly property color greenShadow: "#2c8e72"

        readonly property color white50: "#80ffffff"
        readonly property color baseGrayHighlight15: "#26575757"
        readonly property color baseGrayHighlight40: "#66575757"
        readonly property color faintGray50: "#80e3e3e3"
        readonly property color baseGrayShadow60: "#99252525"
    }

    Item {
        id: dimensions
        readonly property real borderRadius: Screen.width >= 1920 && Screen.height >= 1080 ? 7.5 : 5.0
        readonly property real borderWidth: Screen.width >= 1920 && Screen.height >= 1080 ? 2 : 1
    }

    /*
    SystemPalette { id: sysPalette; colorGroup: SystemPalette.Active }
    readonly property alias colors: colors
    readonly property alias layout: layout
    readonly property alias fonts: fonts
    readonly property alias styles: styles
    readonly property alias effects: effects

    Item {
        id: colors
        readonly property color hifiBlue: "#0e7077"
        readonly property color window: sysPalette.window
        readonly property color dialogBackground: sysPalette.window
        readonly property color inputBackground: "white"
        readonly property color background: sysPalette.dark
        readonly property color text: "#202020"
        readonly property color disabledText: "gray"
        readonly property color hintText: "gray"  // A bit darker than sysPalette.dark so that it is visible on the DK2
        readonly property color light: sysPalette.light
        readonly property alias activeWindow: activeWindow
        readonly property alias inactiveWindow: inactiveWindow
        QtObject {
            id: activeWindow
            readonly property color headerBackground: "white"
            readonly property color headerText: "black"
        }
        QtObject {
            id: inactiveWindow
            readonly property color headerBackground: "gray"
            readonly property color headerText: "black"
        }
     }

    QtObject {
        id: fonts
        readonly property string fontFamily: "Arial"  // Available on both Windows and OSX
        readonly property real pixelSize: 22  // Logical pixel size; works on Windows and OSX at varying physical DPIs
        readonly property real headerPixelSize: 32
    }

    QtObject {
        id: layout
        property int spacing: 8
        property int rowHeight: 40
        property int windowTitleHeight: 48
    }

    QtObject {
        id: styles
        readonly property int borderWidth: 5
        readonly property int borderRadius: borderWidth * 2
    }
    */

    QtObject {
        id: effects
        readonly property int fadeInDuration: 300
    }
}
