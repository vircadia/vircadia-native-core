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
    readonly property alias colorSchemes: colorSchemes
    readonly property alias dimensions: dimensions
    readonly property alias fontSizes: fontSizes
    readonly property alias buttons: buttons
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
        readonly property color lightGrayBackground: "#d4d4d4"
        readonly property color faintGray: "#e3e3e3"
        readonly property color primaryHighlight: "#00b4ef"
        readonly property color blueHighlight: "#00b4ef"
        readonly property color blueAccent: "#1080b8"
        readonly property color redHighlight: "#e2334d"
        readonly property color redAccent: "#b70a37"
        readonly property color greenHighlight: "#1ac567"
        readonly property color greenShadow: "#2c8e72"
        readonly property color black: "#000000"

        readonly property color white50: "#80ffffff"
        readonly property color white30: "#4dffffff"
        readonly property color white25: "#40ffffff"
        readonly property color baseGrayHighlight15: "#26575757"
        readonly property color baseGrayHighlight40: "#66575757"
        readonly property color darkGray30: "#4d121212"
        readonly property color darkGray0: "#00121212"
        readonly property color faintGray50: "#80e3e3e3"
        readonly property color baseGrayShadow60: "#99252525"
        readonly property color tableRowLightOdd: white50
        readonly property color tableRowLightEven: "#1a575757"
        readonly property color tableRowDarkOdd: "#80393939"
        readonly property color tableRowDarkEven: "#a6181818"
    }

    Item {
        id: colorSchemes
        readonly property int light: 0
        readonly property int dark: 1
    }

    Item {
        id: dimensions
        readonly property bool largeScreen: Screen.width >= 1920 && Screen.height >= 1080
        readonly property real borderRadius: largeScreen ? 7.5 : 5.0
        readonly property real borderWidth: largeScreen ? 2 : 1
        readonly property vector2d contentMargin: Qt.vector2d(12, 24)
        readonly property vector2d contentSpacing: Qt.vector2d(8, 12)
        readonly property real textPadding: 8
        readonly property real tablePadding: 12
        readonly property real tableRowHeight: largeScreen ? 26 : 23
    }

    Item {
        id: fontSizes
        readonly property real overlayTitle: dimensions.largeScreen? 16 : 12
        readonly property real tabName: dimensions.largeScreen? 11 : 9
        readonly property real sectionName: dimensions.largeScreen? 11 : 9
        readonly property real inputLabel: dimensions.largeScreen? 11 : 9
        readonly property real textFieldInput: dimensions.largeScreen? 13.5 : 11
        readonly property real tableText: dimensions.largeScreen? 13.5 : 11
        readonly property real buttonLabel: dimensions.largeScreen? 12 : 10
        readonly property real button: dimensions.largeScreen? 12 : 10
        readonly property real listItem: dimensions.largeScreen? 11 : 9
        readonly property real tabularData: dimensions.largeScreen? 11 : 9
        readonly property real logo: dimensions.largeScreen? 15 : 10
        readonly property real code: dimensions.largeScreen? 15 : 10
        readonly property real rootMenu: dimensions.largeScreen? 11 : 9
        readonly property real menuItem: dimensions.largeScreen? 11 : 9
        readonly property real shortcutText: dimensions.largeScreen? 12 : 8
    }

    Item {
        id: buttons
        readonly property int white: 0
        readonly property int blue: 1
        readonly property int red: 2
        readonly property int black: 3
        readonly property var textColor: [ colors.darkGray, colors.white, colors.white, colors.white ]
        readonly property var colorStart: [ "#ffffff", "#00b4ef", "#d42043", "#343434" ]
        readonly property var colorFinish: [ "#afafaf", "#1080b8", "#94132e", "#000000" ]
        readonly property int radius: 5
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
